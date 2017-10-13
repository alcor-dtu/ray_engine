#pragma once
#include <device_common_data.h>
#include <material.h>
#include <device_mesh_data.h>

using optix::rtTex3D;

rtDeclareVariable(MaterialDataCommon, main_material, , );

rtBuffer<MaterialDataCommon, 1> material_buffer;

rtDeclareVariable(int, noise_tex, , );
rtDeclareVariable(int, use_heterogenous_materials, , );

__device__ __forceinline__ MaterialDataCommon get_material(const float3 & hit_pos)
{
	if (use_heterogenous_materials == 1)
	{
		int material_idx = 0;
		float3 obj_space_position = rtTransformPoint(RTtransformkind::RT_WORLD_TO_OBJECT, hit_pos);
		float max_dim = local_bounding_box->maxExtent();
		float3 norm_position = (obj_space_position - local_bounding_box->m_min) / max_dim;
		material_idx = rtTex3D<float4>(noise_tex, norm_position.x, norm_position.y, norm_position.z).x > 0.5 ? 0 : 1;
		material_idx = optix::min(material_idx, (int)material_buffer.size() - 1);
		return material_buffer[material_idx];
	}
	else
		return material_buffer[0];
    
}

__device__ __forceinline__ MaterialDataCommon get_material()
{
    float3 hit_pos = ray.origin + t_hit * ray.direction;
    return get_material(hit_pos);
}
