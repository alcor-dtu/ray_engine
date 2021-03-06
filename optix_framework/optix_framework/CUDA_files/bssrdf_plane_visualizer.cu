#include <device_common.h>
#include <color_utils.h>
#include <ray_tracing_utils.h>
#include <environment_map.h>
#include <math_utils.h>
#include <color_map_utils.h>
#include <scattering_properties.h>
#include <bssrdf_device.h>
#include "optics_utils.h"

using namespace optix; 

// Window variables  
rtBuffer<float4, 2> output_buffer;

rtDeclareVariable(float, reference_bssrdf_theta_i, , );
rtDeclareVariable(BufPtr<ScatteringMaterialProperties>, planar_bssrdf_material_params, , );
rtDeclareVariable(float, reference_bssrdf_rel_ior, , );
rtDeclareVariable(unsigned int, show_false_colors, , );
rtDeclareVariable(unsigned int, channel_to_show, , );
rtDeclareVariable(float, scale_multiplier, , );

RT_PROGRAM void render()
{
	float2 uv = make_float2(launch_index) / make_float2(launch_dim);
	float2 ip = uv * 2 - make_float2(1); // [-1, 1], this is xd, yd

	ip *= -2;

    BSSRDFGeometry geometry;
    geometry.xi = optix::make_float3(0, 0, 0);
    geometry.ni = optix::make_float3(0, 0, 1);
    geometry.xo = optix::make_float3(ip.x, ip.y, 0);
    geometry.no = optix::make_float3(0, 0, 1);
    geometry.wo = geometry.no;

	const float theta_i_rad = reference_bssrdf_theta_i;
	geometry.wi = normalize(optix::make_float3(sinf(theta_i_rad), 0, cosf(theta_i_rad)));

	float fresnel_integral = planar_bssrdf_material_params->C_phi * 4 * M_PIf;

    MaterialDataCommon mat;
    mat.scattering_properties = planar_bssrdf_material_params[0];

    TEASampler sampler(launch_dim.x*launch_index.y + launch_index.x, frame);
	float3 S = fresnel_integral * bssrdf(geometry, 1.0f / reference_bssrdf_rel_ior, mat, BSSRDFFlags::EXCLUDE_OUTGOING_FRESNEL, sampler);
	float S_shown = optix::get_channel(channel_to_show, S) * scale_multiplier;

	float t = optix::clamp((logf(S_shown + 1.0e-10) / 2.30258509299f + 6.0f) / 6.0f, 0.0f, 1.0f);
	float h = optix::clamp((1.0f - t)*2.0f, 0.0f, 0.65f);
	 
	optix::float4 res = make_float4(S_shown);
	if(show_false_colors)
		res = optix::make_float4(hsv2rgb(h, 1.0, 1.0), 1.0);
	output_buffer[launch_index] = res;
	optix_print("%f %f %f", reference_bssrdf_rel_ior, planar_bssrdf_material_params->absorption.x, planar_bssrdf_material_params->scattering.x);

}
 
