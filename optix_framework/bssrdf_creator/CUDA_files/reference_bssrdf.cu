// 02576 OptiX Rendering Framework
// Written by Jeppe Revall Frisvad, 2011
// Copyright (c) DTU Informatics 2011

#include <photon_trace_reference_bssrdf.h>
#include <device_common.h>
#include <md5.h>
#include <material_common.h>
using namespace optix;
 
rtDeclareVariable(BufPtr2D<float>, reference_resulting_flux, , );
rtDeclareVariable(BufPtr2D<float>, reference_resulting_flux_intermediate, , );

rtDeclareVariable(unsigned int, maximum_iterations, , ) = 1e5;
rtDeclareVariable(unsigned int, ref_frame_number, , ) = 1e5;
rtDeclareVariable(unsigned int, reference_bssrdf_samples_per_frame, , );
// Window variables

rtDeclareVariable(BSSRDFRendererData, reference_bssrdf_data, , );
rtDeclareVariable(BufPtr<ScatteringMaterialProperties>, reference_bssrdf_material_params, , );
rtDeclareVariable(float, reference_bssrdf_rel_ior, , );
rtDeclareVariable(OutputShape::Type, reference_bssrdf_output_shape, , );
rtDeclareVariable(BSSRDFSimulatedOptions, reference_bssrdf_simulated_options, , );


//#define USE_HARDCODED_MATERIALS

RT_PROGRAM void reference_bssrdf_camera()
{
	uint idx = launch_index.x;
	SEED_TYPE t;
	init_seed(t, ((unsigned long long)ref_frame_number) * launch_dim.x + idx);
	RND_FUNC(t);
	 
	const float incident_power = 1.0f;
	float theta_i; float r; float theta_s; float albedo; float extinction; float g; float n2_over_n1;
	  
	theta_i = reference_bssrdf_data.mThetai;
	theta_s = reference_bssrdf_data.mThetas.x;
	r = reference_bssrdf_data.mRadius.x;
	n2_over_n1 = reference_bssrdf_rel_ior;
	albedo = reference_bssrdf_material_params->albedo.x;
	extinction = reference_bssrdf_material_params->extinction.x;
	g = reference_bssrdf_material_params->meancosine.x;

	optix::float3 xi, wi, ni, xo, no;
	get_reference_scene_geometry(theta_i, r, theta_s, xi, wi, ni, xo, no);


	// Refraction 

	const float n1_over_n2 = 1.0f / n2_over_n1;
	optix::float3 w12;
	refract(wi, ni, n1_over_n2, w12);

	float flux_t = incident_power / ((float)reference_bssrdf_samples_per_frame);
	optix::float3 xp = xi; 
	optix::float3 wp = w12;

	if (!scatter_photon(reference_bssrdf_output_shape, reference_bssrdf_simulated_options, reference_bssrdf_data, xp, wp, flux_t, reference_resulting_flux_intermediate, xo, n2_over_n1, albedo, extinction, g, t, 0, maximum_iterations))
	{ 
		optix_print("Max iterations reached. Distance %f (%f mfps)\n", length(xp - xo), length(xp - xo) / r);  
	}  
}

RT_PROGRAM void post_process_bssrdf()
{
	reference_resulting_flux[launch_index] = reference_resulting_flux_intermediate[launch_index] / ((float)(ref_frame_number + 1));
}

