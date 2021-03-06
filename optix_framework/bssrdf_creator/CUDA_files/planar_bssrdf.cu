
// 02576 OptiX Rendering Framework
// Written by Jeppe Revall Frisvad, 2011
// Copyright (c) DTU Informatics 2011

#include "host_device_common.h"
#include <device_common.h>
#include <md5.h>
#include <material_common.h>
#include "empirical_bssrdf_common.h"
#include <bssrdf_device.h>
#include "../photon_trace_reference_bssrdf.h"
using namespace optix;
 
rtDeclareVariable(BufPtr2D<float>, planar_resulting_flux, , );
rtDeclareVariable(BufPtr2D<float>, planar_resulting_flux_intermediate, , );
 
rtDeclareVariable(unsigned int, maximum_iterations, , );
rtDeclareVariable(unsigned int, ref_frame_number, , ); 
rtDeclareVariable(unsigned int, reference_bssrdf_samples_per_frame, , );
// Window variables

rtDeclareVariable(BSSRDFRendererData, reference_bssrdf_data, , );

rtDeclareVariable(BufPtr<ScatteringMaterialProperties>, planar_bssrdf_material_params, , );
rtDeclareVariable(float, reference_bssrdf_rel_ior, , );
rtDeclareVariable(OutputShape::Type, reference_bssrdf_output_shape, , );

RT_PROGRAM void reference_bssrdf_camera()
{
	float2 uv = make_float2(launch_index) / make_float2(launch_dim);

	float theta_i = reference_bssrdf_data.mThetai;
	float n2_over_n1 = reference_bssrdf_rel_ior;
	float albedo = planar_bssrdf_material_params->albedo.x;
	float extinction = planar_bssrdf_material_params->extinction.x;
	float g = planar_bssrdf_material_params->meancosine.x;
	float theta_s = reference_bssrdf_data.mThetas.x;
	float r =  reference_bssrdf_data.mRadius.x;

    BSSRDFGeometry geometry;
	get_reference_scene_geometry(theta_i, r, theta_s, geometry.xi, geometry.wi, geometry.ni, geometry.xo, geometry.no);

    float2 angles;
	angles = get_normalized_hemisphere_buffer_angles(reference_bssrdf_output_shape, uv.x, uv.y);//get_bin_center(uv.x, uv.y, reference_bssrdf_data.mPhioBins, reference_bssrdf_data.mThetaoBins);
	angles.y = fmaxf(0.001f, angles.y);
	geometry.wo = optix::make_float3(sinf(angles.y) * cosf(angles.x), sinf(angles.y) * sinf(angles.x), cosf(angles.y));

	const float n1_over_n2 = 1.0f / n2_over_n1;

    MaterialDataCommon mat;
    mat.scattering_properties = planar_bssrdf_material_params[0];
    TEASampler sampler(launch_dim.x*launch_index.y + launch_index.x, 0);
    optix::float3 S = bssrdf(geometry, n1_over_n2, mat, BSSRDFFlags::NO_FLAGS, sampler);
	planar_resulting_flux_intermediate[launch_index] = S.x;
} 
      
RT_PROGRAM void post_process_bssrdf() 
{
	planar_resulting_flux[launch_index] = planar_resulting_flux_intermediate[launch_index];
}
