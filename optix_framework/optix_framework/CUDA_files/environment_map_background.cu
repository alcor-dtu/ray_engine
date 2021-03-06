// 02576 OptiX Rendering Framework
// Written by Jeppe Revall Frisvad, 2011
// Copyright (c) DTU Informatics 2011

#include <device_common.h>
#include <color_utils.h>
#include <environment_map_device.h>

// Standard ray variables
rtDeclareVariable(PerRayData_radiance, prd_radiance, rtPayload, );
rtDeclareVariable(PerRayData_shadow, prd_shadow, rtPayload, );

using optix::rtTex2D;
using optix::float2;
using optix::float4;

_fn void get_environment_map_color(const float3& direction, float3 & color)
{
    const float2 uv = direction_to_uv_coord_cubemap(direction, envmap_properties.lightmap_rotation_matrix);
    color = make_float3(rtTex2D<float4>(envmap_properties.environment_map_tex_id, uv.x, uv.y)) * envmap_properties.lightmap_multiplier;
}

// Miss program returning background color
RT_PROGRAM void miss()
{
  float3 color = optix::make_float3(0.0f);
  if (prd_radiance.flags & RayFlags::USE_EMISSION)
  {
	get_environment_map_color(ray.direction, color);
  }
  prd_radiance.result = color;
  optix_print("Ray miss, hit envmap. Returning color %f %f %f\n", color.x, color.y, color.z);
}

// Miss program returning background color
RT_PROGRAM void miss_shadow()
{
	float3 color = optix::make_float3(0.0f);
	get_environment_map_color(ray.direction, color);
	prd_shadow.emission = color;
	prd_shadow.attenuation = 1.0f;
	optix_print("Shadow ray miss, hit envmap. Returning color %f %f %f\n", color.x, color.y, color.z);
}
