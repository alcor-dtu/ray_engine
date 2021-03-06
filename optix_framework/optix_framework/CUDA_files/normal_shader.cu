// 02576 OptiX Rendering Framework
// Written by Jeppe Revall Frisvad, 2011
// Copyright (c) DTU Informatics 2011

#include <device_common.h>
#include <color_utils.h>
#include <environment_map.h>

using namespace optix;

// Standard ray variables
rtDeclareVariable(PerRayData_radiance, prd_radiance, rtPayload, );

// Variables for shading
rtDeclareVariable(float3, shading_normal, attribute shading_normal, );

// Any hit program for shadows
RT_PROGRAM void any_hit_shadow() { rtTerminateRay(); }

// Closest hit program for drawing shading normals
RT_PROGRAM void shade()
{
  float3 normal = rtTransformNormal(RT_OBJECT_TO_WORLD, shading_normal);
  prd_radiance.result = normalize(normal) * 0.5f + 0.5f;
}