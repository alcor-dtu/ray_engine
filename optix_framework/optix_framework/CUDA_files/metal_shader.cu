// 02576 OptiX Rendering Framework
// Written by Jeppe Revall Frisvad, 2011
// Copyright (c) DTU Informatics 2011

#include <device_common.h>
#include <color_utils.h>
#include <optics_utils.h>
#include <material_device.h>
#include <environment_map.h>
#include <ray_tracing_utils.h>

using namespace optix;

// Standard ray variables
rtDeclareVariable(PerRayData_radiance, prd_radiance, rtPayload, );
rtDeclareVariable(PerRayData_shadow,   prd_shadow,   rtPayload, );

// Variables for shading
rtDeclareVariable(float3, shading_normal, attribute shading_normal, );
rtDeclareVariable(float2, texcoord, attribute texcoord, );

// Any hit program for shadows
RT_PROGRAM void any_hit_shadow() {
    const MaterialDataCommon & material = get_material(texcoord);
    float3 emission = make_float3(rtTex2D<float4>(material.ambient_map, texcoord.x, texcoord.y));
	shadow_hit(prd_shadow,emission);
}

RT_PROGRAM void shade()
{
  float3 hit_pos = ray.origin + t_hit * ray.direction;
  float3 normal = rtTransformNormal(RT_OBJECT_TO_WORLD, shading_normal);
  float3 ffnormal = faceforward(normal, -ray.direction, normal);

    if(prd_radiance.depth < max_depth)
    {
        float3 ior = get_material(texcoord).index_of_refraction;
		float3 imag = get_material(texcoord).scattering_properties.absorption * optix::make_float3(700e-9, 550e-9, 425e-9) / (4 * M_PIf);

        float3 R = fresnel_complex_R(-ray.direction, ffnormal, ior*ior, imag*imag);
        PerRayData_radiance prd_new = prepare_new_pt_payload(prd_radiance);
		float3 new_dir = reflect(ray.direction, ffnormal);

		optix::Ray new_ray(hit_pos, new_dir,  RayType::RADIANCE, scene_epsilon, RT_DEFAULT_MAX);
        rtTrace(top_object, new_ray, prd_new);
        prd_radiance.result = R * prd_new.result;
    }
    else
    {
        prd_radiance.result = make_float3(0.0f);
    }
}
