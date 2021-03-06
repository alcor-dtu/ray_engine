#include "default_shader.h"
#include "object_host.h"
#include "shader_factory.h"

std::vector<ShaderInfo> DefaultShader::mDefaultShaders = 
{
	ShaderInfo(0, "constant_shader.cu", "Constant"),
	ShaderInfo(1, "lambertian_shader.cu", "Lambertian"),
	ShaderInfo(3, "mirror_shader.cu", "Mirror"),
	ShaderInfo(4, "glass_shader.cu", "Glass"),
	ShaderInfo(5, "normal_shader.cu", "Normals"),
	ShaderInfo(6, "uv_shader.cu", "UVs"),
	ShaderInfo(7, "wireframe_shader.cu", "Wireframe"),
    ShaderInfo(8, "material_labels_shader.cu", "Material labels"),
	ShaderInfo(11, "metal_shader.cu", "Metal")
};

void DefaultShader::initialize_shader(optix::Context ctx)
{
    Shader::initialize_shader(ctx);
}

void DefaultShader::initialize_material(MaterialHost &object)
{
    Shader::initialize_material(object);
}

template <>
void DefaultShader::serialize<cereal::XMLOutputArchiveOptix>(cereal::XMLOutputArchiveOptix& archive)
{
    archive(cereal::base_class<Shader>(this));
}

template <>
void DefaultShader::serialize<cereal::XMLInputArchiveOptix>(cereal::XMLInputArchiveOptix& archive)
{
    archive(cereal::base_class<Shader>(this));

}

