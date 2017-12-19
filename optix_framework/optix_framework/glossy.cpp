#include "glossy.h"
#include "folders.h"
#include "parameter_parser.h"
#include "mesh.h"
#include "brdf_utils.h"
#include "merl_common.h"
#include <ImageLoader.h>
#include "host_material.h"
#include "scattering_material.h"
#include <algorithm>

void GlossyShader::initialize_shader(optix::Context context)
{
    Shader::initialize_shader(context);
    blinn_exponent = ConfigParameters::get_parameter<float>("glossy", "blinn_exp", 1.0f);
    anisotropic_exp = ConfigParameters::get_parameter<optix::float2>("glossy", "anisotropic_exp", optix::make_float2(.5f, 1.0f));
    x_axis_anisotropic = ConfigParameters::get_parameter<optix::float3>("glossy", "x_axis_anisotropic", optix::make_float3(1.0f, 0.0f, 0.0f));
    get_merl_brdf_list(Folders::merl_database_file.c_str(), brdf_names);
    merl_folder = Folders::merl_folder;
    merl_correction = ConfigParameters::get_parameter<optix::float3>("glossy", "merl_multiplier", optix::make_float3(1.0f), "Multiplication factor for MERL materials. Premultiplied on sampling the brdf.");
    Logger::debug << "Merl correction factor: " << std::to_string(merl_correction.x) << " " << std::to_string(merl_correction.y) << " " << std::to_string(merl_correction.z) << std::endl;
    use_merl_brdf = ConfigParameters::get_parameter<bool>("config", "use_merl_brdf", false, "configure the ray tracer to try to use the MERL brdf database whenever possible.");
}

void GlossyShader::set_data(Mesh& object)
{
    object.mMaterial["exponent_blinn"]->setFloat(blinn_exponent);
    object.mMaterial["exponent_aniso"]->setFloat(anisotropic_exp);
    object.mMaterial["object_x_axis"]->setFloat(x_axis_anisotropic);
    object.mMaterial["merl_brdf_multiplier"]->setFloat(merl_correction);
}

void GlossyShader::initialize_mesh(Mesh& object)
{
    Shader::initialize_mesh(object);
    set_data(object);
	std::string n = object.get_main_material()->get_name();

	std::string n_ext = n + ".binary";
    MERLBrdf * mat = nullptr;
    auto found = std::find(std::begin(brdf_names), std::end(brdf_names), n_ext);
    if (merl_database.count(n) != 0)
    {
        mat = &merl_database[n];
    }
    else if (found != brdf_names.end())
    {
        merl_database[n] = MERLBrdf();
        mat = &merl_database[n];
        read_brdf_f(Folders::merl_folder, n_ext, mat->data);
        mat->name = n;
        mat->reflectance = integrate_brdf(mat->data, 100000);
    }
    else
    {
        Logger::warning << "Equivalent MERL Material " << n << " not found." << std::endl;
    }
    auto optix_mat = object.mMaterial;

    uint has_merl = mat == nullptr ? 0 : 1;
	optix::float3 reflectance = mat == nullptr ? optix::make_float3(0) : mat->reflectance;
    size_t buffer_size = mat == nullptr ? 1 : mat->data.size();


    optix_mat["has_merl_brdf"]->setUint(has_merl);
    optix::Buffer buff = optix_mat->getContext()->createBuffer(RT_BUFFER_INPUT);
    buff->setFormat(RT_FORMAT_FLOAT);
    buff->setSize(buffer_size);
    if (mat != nullptr)
    {
        void* b = buff->map();
        memcpy(b, mat->data.data(), buffer_size * sizeof(float));
        buff->unmap();
    }
    optix_mat["merl_brdf_buffer"]->setBuffer(buff);
    optix_mat["diffuse_map"]->setTextureSampler(createOneElementSampler(optix_mat->getContext(), reflectance));
    optix_mat["merl_brdf_buffer"]->setBuffer(buff);

}

void GlossyShader::pre_trace_mesh(Mesh& object)
{
    Shader::pre_trace_mesh(object);
    set_data(object);
}
