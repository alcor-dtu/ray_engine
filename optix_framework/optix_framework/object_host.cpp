//
// Created by alcor on 4/19/18.
//

#include "object_host.h"
#include "shader_factory.h"
#include "scattering_material.h"
#include <algorithm>
#include "immediate_gui.h"
#include "file_dialogs.h"
#include "image_loader.h"
#include "obj_loader.h"
#include "scene_gui.h"
#include "scene.h"

std::unique_ptr<Texture> Object::create_label_texture(optix::Context ctx, const std::unique_ptr<Texture>& ptr)
{
    std::unique_ptr<Texture> ret = std::make_unique<Texture>(ctx, Texture::INT, 1);
    ret->set_size(ptr->get_width(), ptr->get_height());
    for(size_t i = 0; i < ptr->get_width(); i++)
    {
        for(size_t j = 0; j < ptr->get_height(); j++)
        {
            float value = ((optix::float4*)ptr->get_texel_ptr(i,j))->x;
            int val = value > 0.5f ? 1 : 0;
            ret->set_texel_ptr(&val, i, j);
        }
    }
    ret->update();
    ret->get_sampler()->setFilteringModes(RT_FILTER_NEAREST, RT_FILTER_NEAREST, RT_FILTER_NONE);
    return ret;
}

Object::Object(optix::Context ctx) : mScene(nullptr), mContext(ctx)
{
    static int id = 0;
    mMeshID = id++;
    mReloadMaterials = mReloadGeometry = true;

    mMaterialSelectionTexture = std::make_unique<Texture>(ctx, Texture::FLOAT, 4);
    mMaterialSelectionTexture->set_size(1);
    optix::float4 default_label = optix::make_float4(0.0f);
    mMaterialSelectionTexture->set_data(&default_label.x, 4 * sizeof(float));
    mMaterialSelectionTextureLabel = create_label_texture(ctx, mMaterialSelectionTexture);
}

void Object::init(const char *name, std::unique_ptr<Geometry> geometry, std::shared_ptr<MaterialHost>& material)
{
    mMeshName = name;
    mGeometry = std::move(geometry);
    mMaterials.resize(1);
    mMaterials[0] = material;
    create_and_bind_optix_data();
}

void Object::load_materials()
{
    auto id = mMaterialSelectionTextureLabel->get_id();
    mGeometryInstance["material_selector"]->setUserData(sizeof(TexPtr), &id);
    bool one_material_changed = std::any_of(mMaterials.begin(), mMaterials.end(), [](const std::shared_ptr<MaterialHost>& mat) { return mat->has_changed();  });
    mReloadMaterials |= one_material_changed;

    size_t n = mMaterials.size();
    mMaterialBuffer->setSize(n);

    if(mReloadMaterials)
    {
        mMaterialBuffer->setSize(mMaterials.size());

        mMaterialSelectionTextureLabel = create_label_texture(mContext, mMaterialSelectionTexture);

        create_and_bind_optix_data();

        for(auto & m : mMaterials)
        {
            m->scene = mScene;
            m->reload_shader();
        }

        mGeometryInstance->setMaterialCount((unsigned int)n);

        MaterialDataCommon* data = reinterpret_cast<MaterialDataCommon*>(mMaterialBuffer->map());
        for (int i = 0; i < n; i++)
        {
            mGeometryInstance->setMaterial(i, mMaterials[i]->get_optix_material());
            memcpy(&data[i], &mMaterials[i]->get_data(), sizeof(MaterialDataCommon));
        }
        mMaterialBuffer->unmap();
        mGeometryInstance["main_material"]->setUserData(sizeof(MaterialDataCommon), &data[0]);
    }
    mGeometryInstance["material_buffer"]->setBuffer(mMaterialBuffer);

    for(auto & m : mMaterials)
    {
        m->load_shader();
    }

    mReloadMaterials = false;
}

void Object::load_geometry()
{
    if (!mReloadGeometry)
        return;

	mAcceleration->markDirty();
	mScene->get_acceleration()->markDirty();
    create_and_bind_optix_data();

    BufPtr<optix::Aabb> bptr = BufPtr<optix::Aabb>(mGeometry->get_bounding_box_buffer()->getId());
    mGeometryInstance["local_bounding_box"]->setUserData(sizeof(BufPtr<optix::Aabb>), &bptr);
    mGeometryInstance["current_geometry_node"]->set(mTransform->get_transform());
    mGeometry->load();
	mGeometry->load(mGeometryInstance);
    mReloadGeometry = false;
}



void Object::load_transform()
{
    if (!mTransform)
    {
        mTransform = std::make_unique<Transform>(mContext);
    }
    if (mTransform->has_changed())
    {
        mTransform->load();
        mAcceleration->markDirty();
        if(transform_changed_event != nullptr)
            transform_changed_event();
    }
}


void Object::load()
{
    load_geometry();
    load_materials();
    load_transform();
}

void Object::create_and_bind_optix_data()
{
    if (!mGeometryInstance)
    {
        mGeometryInstance = mContext->createGeometryInstance();
    }

    if (mTransform == nullptr)
    {
        mTransform = std::make_unique<Transform>(mContext);
    }

    if (!mGeometryGroup)
    {
        mGeometryGroup = mContext->createGeometryGroup();
        mGeometryGroup->setChildCount(1);
        mGeometryGroup->setChild(0, mGeometryInstance);
        mTransform->get_transform()->setChild(mGeometryGroup);
    }

    if (!mMaterialBuffer.get())
    {
        mMaterialBuffer = create_buffer<MaterialDataCommon>(mContext);
    }

    if(!mAcceleration.get())
    {
        mAcceleration = mContext->createAcceleration(std::string("Trbvh"));
        mAcceleration->setProperty("refit", "0");
        mAcceleration->setProperty("vertex_buffer_name", "vertex_buffer");
        mAcceleration->setProperty("index_buffer_name", "vindex_buffer");
        mGeometryGroup->setAcceleration(mAcceleration);
    }

    if (mGeometryInstance->getGeometry().get() == nullptr && mGeometry != nullptr)
    {
        mGeometryInstance->setGeometry(mGeometry->get_geometry());
        mAcceleration->markDirty();
    }

}

void Object::add_material(std::shared_ptr<MaterialHost> material)
{
    material->scene = mScene;
    mMaterials.push_back(material);
    mMaterialBuffer->setSize(mMaterials.size());
    mReloadMaterials = true;
    load_materials();
}

void Object::remove_material(int material_id)
{
    if (material_id >= 0 && material_id < mMaterials.size())
    {
        mMaterials.erase(mMaterials.begin() + material_id);
        mReloadMaterials = true;
        load_materials();
    }
}


bool Object::on_draw()
{
    ImmediateGUIDraw::PushItemWidth(200);
    bool changed = false;
	std::string tree = mMeshName;
	std::string a = " ID: " + std::to_string(mMeshID) + "##meshid" + std::to_string(mMeshID);
	tree += a;

	if (ImmediateGUIDraw::TreeNode(tree.c_str()))
    {
		ImmediateGUIDraw::InputString((std::string("Name##Name") + mMeshName).c_str(), mMeshName, ImGuiInputTextFlags_EnterReturnsTrue);

        if (ImmediateGUIDraw::TreeNode((std::string("Transform##Transform") + mMeshName).c_str()))
        {
            changed |= mTransform->on_draw();
            ImmediateGUIDraw::TreePop();
        }

        if (ImmediateGUIDraw::TreeNode((std::string("Geometry##Geometry") + mMeshName).c_str()))
        {
            bool g_changed = mGeometry->on_draw();
            changed |= g_changed;
            mReloadGeometry = g_changed;
            ImmediateGUIDraw::TreePop();
        }

        if (ImmediateGUIDraw::TreeNode((std::string("Materials##Materials") + mMeshName).c_str()))
        {
            if (ImmediateGUIDraw::TreeNode((std::string("Material selector##Materialselector") + mMeshName).c_str()))
            {
                mMaterialSelectionTextureLabel->on_draw();
                if (ImmediateGUIDraw::Button("Load material selection texture..."))
                {
                    std::string d;
                    if (Dialogs::open_file_dialog(d))
                    {
                        mMaterialSelectionTexture = loadTexture(mContext, d, optix::make_float4(0));                       
                        load_materials();
                    }
                }
                ImmediateGUIDraw::TreePop();
            }

            if(ImmediateGUIDraw::Button("Add material"))
            {
                add_material(get_default_material(mContext));
                load_materials();
            }

            if (ImGui::Button("Remove material..."))
                ImGui::OpenPopup("removematerialpopup");

            if (ImGui::BeginPopup("removematerialpopup"))
            {
                for (int i = 0; i < mMaterials.size(); i++)
                {
                    if (ImGui::Selectable(mMaterials[i]->get_name().c_str()))
                    {
                        remove_material(i);
                    }
                }
                ImGui::EndPopup();
            }

            for (int i = 0; i < mMaterials.size(); i++)
            {
                auto m = mMaterials[i];
                if(m->on_draw(mMeshName))
                {
                    changed = true;
                    mReloadMaterials = true;
                }
            }
            ImmediateGUIDraw::TreePop();
        }

        ImmediateGUIDraw::TreePop();
    }
    return changed;
}

void Object::pre_trace()
{
    for(auto & m : mMaterials)
    {
        m->get_shader().pre_trace_mesh(*this);
    }
}

void Object::post_trace()
{
    for(auto & m : mMaterials)
    {
        m->get_shader().post_trace_mesh(*this);
    }
}

void Object::reload_materials()
{
    mReloadMaterials = true;
}

Object::~Object()
{
    mGeometry.reset();
    mTransform.reset();
    for(auto & m : mMaterials)
    {
        m.reset();
    }
    mGeometryInstance->destroy();
    mGeometryGroup->destroy();
    mAcceleration->destroy();
    mMaterialBuffer->destroy();
}
