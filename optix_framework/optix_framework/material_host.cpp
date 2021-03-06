#include "material_host.h"
#include "material_library.h"
#include "scattering_material.h"
#include <algorithm>
#include "immediate_gui.h"
#include "obj_loader.h"

#include "optics_utils.h"
#include "optix_host_utils.h"
#pragma warning (disable : 4244)
#pragma warning (disable : 4305)
#include <quantized_diffusion_helpers.h>
#include "logger.h"
#include "shader_factory.h"

using optix::float3;

bool findAndReturnMaterial(const std::string name, ScatteringMaterial & s)
{
	std::string to_compare = name;
	std::transform(to_compare.begin(), to_compare.end(), to_compare.begin(), ::tolower);
    auto ss = std::find_if(ScatteringMaterial::defaultMaterials.begin(), ScatteringMaterial::defaultMaterials.end(), [&](ScatteringMaterial & v){ return name.compare(v.get_name()) == 0; });
    if (ss != ScatteringMaterial::defaultMaterials.end())
        s = *ss;
    return ss != ScatteringMaterial::defaultMaterials.end();
}

std::string create_id_str(std::string name, std::string id)
{
	return name + "##" + name + id;
}

bool MaterialHost::on_draw(std::string myid = "")
{
	bool changed = false;
	std::string id = myid + "Material" + std::to_string(mMaterialID);
	std::string newgroup = mMaterialName + " (ID: " + std::to_string(mMaterialID) + ") ##" + id;

	if (ImmediateGUIDraw::TreeNode(create_id_str(newgroup, id).c_str()))
	{
		ImmediateGUIDraw::InputString((std::string("Name##Name") + mMaterialName).c_str(), mMaterialName, ImGuiInputTextFlags_EnterReturnsTrue);

        auto map = ShaderFactory::get_map();
        std::vector<std::string> vv;
        std::vector<int> illummap;
        int initial = mShader->get_illum();
        int starting = 0;
        for (auto& p : map)
        {
            vv.push_back(p.second->get_name());
            illummap.push_back(p.first);
            if (p.first == initial)
                starting = (int)illummap.size()-1;
        }
        std::vector<const char *> v;
        for (auto& c : vv) v.push_back(c.c_str());

        int selected = starting;
        if (ImmediateGUIDraw::TreeNode(create_id_str("Shader", id).c_str()))
        {
            if (ImGui::Combo(create_id_str("Set Shader", id).c_str(), &selected, v.data(), (int)v.size(), (int)v.size()))
            {
                changed = true;
                set_shader(illummap[selected]);
            }

            bool shader_changed = mShader->on_draw();
			if(shader_changed)
				mReloadShader = true;

			changed |= shader_changed;
            ImmediateGUIDraw::TreePop();
        }

        if (ImmediateGUIDraw::TreeNode(create_id_str("Properties", id).c_str()))
        {
            ka_gui = *(optix::float4*)textures[0]->get_texel_ptr(0,0);
            kd_gui = *(optix::float4*)textures[1]->get_texel_ptr(0,0);
            ks_gui = *(optix::float4*)textures[2]->get_texel_ptr(0,0);

            if (ImmediateGUIDraw::InputFloat3(create_id_str("Ambient", id).c_str(), &ka_gui.x))
            {
                textures[0]->set_texel_ptr(&ka_gui, 0, 0);
                textures[0]->update();
                if (optix::dot(optix::make_float3(ka_gui), optix::make_float3(1)) > 0)
                    mIsEmissive = true;
            }
            if (ImmediateGUIDraw::InputFloat3(create_id_str("Diffuse", id).c_str(), &kd_gui.x))
            {
                textures[1]->set_texel_ptr(&kd_gui, 0, 0);
                textures[1]->update();
            }
            if (ImmediateGUIDraw::InputFloat3(create_id_str("Specular", id).c_str(), &ks_gui.x))
            {
               textures[2]->set_texel_ptr(&ks_gui, 0, 0);
                textures[2]->update();
            }

            static bool anisotropic = true;
            if (anisotropic)
            {
                if (ImmediateGUIDraw::InputFloat2(create_id_str("Roughness", id).c_str(), &mMaterialData.roughness.x))
                {
                    changed = true;
                }
            }
            else
            {
                if (ImmediateGUIDraw::InputFloat(create_id_str("Roughness", id).c_str(), &mMaterialData.roughness.x))
                {
                    changed = true;
                    mMaterialData.roughness.y = mMaterialData.roughness.x;
                }
            }
            ImmediateGUIDraw::SameLine();
            ImmediateGUIDraw::Checkbox("Anisotropic", &anisotropic);

			if (ImmediateGUIDraw::InputFloat(create_id_str("Anisotropy angle", id).c_str(), &mMaterialData.anisotropy_angle))
			{
				changed = true;
			}

			float f = dot(mMaterialData.index_of_refraction, optix::make_float3(1)) / 3.0f;
            if (ImmediateGUIDraw::InputFloat(create_id_str("Index of refraction", id).c_str(), &f))
            {
				mMaterialData.index_of_refraction = optix::make_float3(f);
				changed = true;
			}
            changed |= scattering_material->on_draw(myid);
            ImmediateGUIDraw::TreePop();
        }
        ImmediateGUIDraw::TreePop();
	}
	return changed;
}

void get_relative_ior(const MPMLMedium& med_in, const MPMLMedium& med_out, optix::float3& eta, optix::float3& kappa)
{
	const float3& eta1 = med_in.ior_real;
	const float3& eta2 = med_out.ior_real;
	const float3& kappa1 = med_in.ior_imag;
	const float3& kappa2 = med_out.ior_imag;

	float3 ab = (eta1 * eta1 + kappa1 * kappa1);
	eta = (eta2 * eta1 + kappa2 * kappa1) / ab;
	kappa = (eta1 * kappa2 - eta2 * kappa1) / ab;
}

bool is_valid_material(ObjMaterial& mat)
{
	return mat.scale > 0.0f && dot(mat.absorption, optix::make_float3(1)) >= 0.0f && dot(mat.asymmetry, optix::make_float3(1)) >= 0.0f && dot(mat.scattering, optix::make_float3(1)) >= 0.0f;
}

MaterialHost::MaterialHost(optix::Context & context, ObjMaterial& mat) : MaterialHost(context)
{
	ObjMaterial * data = &mat;
	if (mat.illum == -1)
	{
		if (user_defined_material != nullptr)
		{
			data = user_defined_material.get();
		}
		else
		{
			Logger::error << "Need to define a user material if using illum -1" << std::endl;
		}
	}

	mMaterialName = data->name;
	std::transform(mMaterialName.begin(), mMaterialName.end(), mMaterialName.begin(), ::tolower);	


	mMaterialData.ambient_map = data->ambient_tex->get_id();
	mMaterialData.diffuse_map = data->diffuse_tex->get_id();
	mMaterialData.illum = data->illum;
	mMaterialData.roughness = optix::make_float2(sqrtf(2.0f/(data->shininess + 2.0f))); // Karis conversion technique
	mMaterialData.specular_map = data->specular_tex->get_id();
	mMaterialData.anisotropy_angle = 0;

	textures.push_back(std::move(mat.ambient_tex));
	textures.push_back(std::move(mat.diffuse_tex));
	textures.push_back(std::move(mat.specular_tex));

	if (is_valid_material(*data))
	{
		Logger::info << mMaterialName << " is a valid obj material. Using obj parameters. " << std::endl;
		mMaterialData.index_of_refraction = optix::make_float3(mat.ior);
		scattering_material = std::make_unique<ScatteringMaterial>(mat.absorption, mat.scattering, mat.asymmetry, mat.scale, mMaterialName.c_str());
	}
	else
	{
		Logger::info << "Looking for material properties for material " << mMaterialName << "..." << std::endl;
		ScatteringMaterial def = ScatteringMaterial(DefaultScatteringMaterial::Marble);

		if (findAndReturnMaterial(mMaterialName, def))
		{
			Logger::info << "Material found in default materials. " << std::endl;
			scattering_material = std::make_unique<ScatteringMaterial>(def);
			mMaterialData.index_of_refraction = optix::make_float3(data->ior == 0.0f ? 1.3f : data->ior);
			Logger::debug << std::to_string(scattering_material->get_scale()) << std::endl;
		}
		else if (MaterialLibrary::media.count(mMaterialName) != 0)
		{
			Logger::info << "Material found in mpml file. " << std::endl;
			MPMLMedium mat = MaterialLibrary::media[mMaterialName];
			MPMLMedium air = MaterialLibrary::media["air"];
			float3 eta, kappa;
			get_relative_ior(air, mat, eta, kappa);
			mMaterialData.index_of_refraction = eta;
			scattering_material = std::make_unique<ScatteringMaterial>(mat.absorption, mat.scattering, mat.asymmetry);
		}
		else if (MaterialLibrary::interfaces.count(mMaterialName) != 0)
		{
			Logger::info << "Material found in mpml file as interface. " << std::endl;
			MPMLInterface interface = MaterialLibrary::interfaces[mMaterialName];
			float3 eta, kappa;
			get_relative_ior(*interface.med_out, *interface.med_in, eta, kappa);
			mMaterialData.index_of_refraction = eta;
			scattering_material = std::make_unique<ScatteringMaterial>(interface.med_in->absorption, interface.med_in->scattering, interface.med_in->asymmetry);
		}
		else
		{
			Logger::warning << "Scattering properties for material " << mMaterialName << "  not found. " << std::endl;
			mMaterialData.index_of_refraction = optix::make_float3(1.0f);
			scattering_material = std::make_unique<ScatteringMaterial>(optix::make_float3(1), optix::make_float3(0), optix::make_float3(1));
		}
	}

	mIsEmissive = mat.is_emissive;

	if (!mMaterial)
	{
		mMaterial = mContext->createMaterial();
	}

    set_shader(mMaterialData.illum);

}


const MaterialDataCommon& MaterialHost::get_data()
{
	if (mHasChanged || scattering_material->hasChanged())
	{
		float relative_ior = dot(mMaterialData.index_of_refraction, optix::make_float3(1)) / 3.0f;
		scattering_material->computeCoefficients(relative_ior);
		mHasChanged = false;
		mMaterialData.scattering_properties = scattering_material->get_data();		
	}
    return mMaterialData;
}


bool MaterialHost::has_changed()
{
	return mHasChanged || scattering_material->hasChanged();
}

std::unique_ptr<ObjMaterial> MaterialHost::user_defined_material = nullptr;

void MaterialHost::set_default_material(ObjMaterial mat)
{
	user_defined_material = std::make_unique<ObjMaterial>(mat);
}

bool MaterialHost::is_emissive()
{
    return mIsEmissive;
}

MaterialHost::MaterialHost(optix::Context &ctx) : mContext(ctx), mMaterialName(), mMaterialData()
{
	static int id = 0;
	mMaterialID = id++;
	mMaterial = mContext->createMaterial();
}

void MaterialHost::reload_shader()
{
	mReloadShader = true;
}

void MaterialHost::set_shader(int illum)
{
	mShader = ShaderFactory::get_shader(illum);
	mReloadShader = true;
}


void MaterialHost::set_shader(const std::string &source) {
	if(mShader != nullptr)
	{
		mShader->set_source(source);
		mReloadShader = true;
	}
}

void MaterialHost::load_shader()
{
    if(mShader == nullptr)
    {
        set_shader(mMaterialData.illum);
    }

    if (mReloadShader)
    {
        mShader->initialize_material(*this);
        mReloadShader = false;
    }

    mShader->load_data(*this);
}

void MaterialHost::load_and_construct(cereal::XMLInputArchiveOptix& archive, cereal::construct<MaterialHost>& construct)
{
    construct(archive.get_context());
    archive(cereal::make_nvp("name", construct->mMaterialName));
    archive(cereal::make_nvp("material_data", construct->mMaterialData));
    archive(cereal::make_nvp("scattering_material", construct->scattering_material));
    archive(cereal::make_nvp("is_emissive", construct->mIsEmissive));
    archive(cereal::make_nvp("textures", construct->textures));

    std::string shader_type;
    archive(cereal::make_nvp("shader_type", shader_type));

    if (shader_type == "illum")
    {
        int illum;
        archive(cereal::make_nvp("shader", illum));
        construct->mShader = ShaderFactory::get_shader(illum);
    }
    else
    {
        archive(cereal::make_nvp("shader", construct->mShader));
    }
    construct->mReloadShader = true;
    construct->mMaterialData.ambient_map = construct->textures[0]->get_id();
    construct->mMaterialData.diffuse_map = construct->textures[1]->get_id();
    construct->mMaterialData.specular_map = construct->textures[2]->get_id();
}

MaterialHost::~MaterialHost()
{
    if(mShader != nullptr && mMaterial.get() != nullptr)
    {
        mShader->tear_down_material(*this);
        mMaterial->destroy();
    }
}
