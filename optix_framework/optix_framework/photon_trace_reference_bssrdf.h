#pragma once
#include <device_common_data.h>
#include <random.h>
#include <sampling_helpers.h>
#include <environment_map.h>
#include "light.h"
#include "device_environment_map.h"
#include "optical_helper.h"
#include "phase_function.h"

__forceinline__ __device__ bool intersect_plane(const optix::float3 & plane_origin, const optix::float3 & plane_normal, const optix::Ray & ray, float & intersection_distance)
{
	float denom = optix::dot(plane_normal, ray.direction);
	if (abs(denom) < 1e-6)
		return false; // Parallel: none or all points of the line lie in the plane.
	intersection_distance = optix::dot((plane_origin - ray.origin), plane_normal) / denom;
	return intersection_distance > ray.tmin && intersection_distance < ray.tmax;
}

__forceinline__ __device__ void store_values_in_buffer(const float cos_theta_o, const float phi_o, const float flux_E, optix::buffer<float, 2> & resulting_flux)
{
	const optix::size_t2 bins = resulting_flux.size();
	float phi_o_normalized = normalize_angle(phi_o) / (2.0f * M_PIf);
	const float theta_o_normalized = acosf(cos_theta_o) / (M_PIf * 0.5f);

	optix_assert(theta_o_normalized >= 0.0f && theta_o_normalized < 1.0f);
	optix_assert(phi_o_normalized < 1.0f);
	optix_assert(phi_o_normalized >= 0.0f);

	optix::float2 coords = make_float2(phi_o_normalized, theta_o_normalized);
	optix::uint2 idxs = make_uint2(coords * make_float2(bins));
	optix_assert(flux_E >= 0.0f);
	optix_assert(!isnan(flux_E));


	if (!isnan(flux_E))
		atomicAdd(&resulting_flux[idxs], flux_E);
}

// Returns true if the photon has been absorbed or has exited the medium, false otherwise
__forceinline__ __device__ bool scatter_photon(optix::float3& xp, optix::float3& wp, float & flux_t, optix::buffer<float,2> & resulting_flux, const float3& xo, const float n1_over_n2, const float albedo, const float extinction, const float g, optix::uint & t, int starting_it, int executions)
{
//	optix_print("\nDoing %d iterations. wp = %f %f %f - %d (%d to %d) optical %f %f %f %f flux %f,\n", executions, wp.x, wp.y, wp.z, t, starting_it, executions, albedo, extinction, g, n1_over_n2, flux_t);
//	optix_print("xp %f %f %f xo %f %f %f\n", xp.x, xp.y, xp.z, xo.x, xo.y, xo.z);

	
	// Geometry
	const optix::float3 xi = optix::make_float3(0, 0, 0);
	const optix::float3 ni = optix::make_float3(0, 0, 1);
	const optix::float3 no = ni;
	const float n2_over_n1 = 1.0f / n1_over_n2;

	int i;
	for (i = starting_it; i < starting_it + executions; i++)
	{
		float rand = 1.0f - rnd(t); // Avoids zero thus infinity.
		float d = -log(rand) / extinction;
		float intersection_distance;
		optix::Ray ray = optix::make_Ray(xp, wp, 0, scene_epsilon, d);
		optix_assert(xp.z < 1e-6);
		optix_assert(xp.z > -INFINITY);

		if (!intersect_plane(xi, ni, ray, intersection_distance))
		{
			// Still within the medium...
			xp = xp + wp * d;
			optix::float3 d_vec = xo - xp;
			const float d_vec_len = optix::length(d_vec);
			d_vec = d_vec / d_vec_len; // Normalizing

			optix_assert(optix::dot(d_vec, no) > 0.0f);

			const float cos_theta_21 = optix::max(optix::dot(d_vec, no), 0.0f); // This should be positive
			const float cos_theta_21_sqr = cos_theta_21*cos_theta_21;

			// Note: we flip the relative ior because we are going outside from inside
			const float sin_theta_o_sqr = n2_over_n1*n2_over_n1*(1.0f - cos_theta_21_sqr);
			//optix_print("(%d) - pos %f %f dir %f %f\n", i, xp.x, xp.y, d_vec.x, d_vec.y);

			if (sin_theta_o_sqr >= 1.0f) // Total internal reflection, no accumulation
			{
				optix_print("(%d) Internal refr.\n", i);
			}
			else
			{
				float cos_theta_o = sqrt(1.0f - sin_theta_o_sqr);
				const float F_t = 1.0f - fresnel_R(cos_theta_21, n2_over_n1); // assert F_t < 1
				float phi_21 = atan2f(d_vec.y, d_vec.x);

				optix_assert(F_t < 1.0f);

				// Store atomically in appropriate spot.

				float flux_E = flux_t * albedo* eval_HG(optix::dot(d_vec, wp), g) *expf(-extinction*d_vec_len) * F_t;
				float phi_o = phi_21; // 

				if (i > 0)
				{
					store_values_in_buffer(cos_theta_o, phi_o, flux_E, resulting_flux);
				}
				optix_print("(%d) Scattering.  %f\n", i, flux_E);
				
			}

			float absorption_prob = rnd(t);
			if (absorption_prob > albedo)
			{
				optix_print("(%d) Absorption.\n", i);
				return true;
			}
			// We scatter, so we choose a new direction sampling the phase function
			wp = optix::normalize(sample_HG(wp, g, t));
		}
		else
		{
			// We are going out!
			const optix::float3 surface_point = xp + wp * intersection_distance;
			optix_assert(optix::dot(wp, no) > 0);
			const float cos_theta_p = optix::max(optix::dot(wp, no), 0.0f);
			const float cos_theta_p_sqr = cos_theta_p*cos_theta_p;

			const float sin_theta_o_sqr = n2_over_n1*n2_over_n1*(1.0f - cos_theta_p_sqr);
			const float cos_theta_o = sqrt(1.0f - sin_theta_o_sqr);
			const float F_r = fresnel_R(cos_theta_o, n2_over_n1); // assert F_t < 1
			if (sin_theta_o_sqr >= 1.0f) // Total internal reflection,
			{
				xp = surface_point;
				wp = reflect(wp, -no); // Reflect and turn to face inside.
				
				optix_assert(F_r < 1.0f);
				flux_t *= F_r;
				optix_print("(%d) Interally refracting.\n", i);
			}
			else
			{
				const float F_t = 1.0f - F_r;
				flux_t *= F_t;
				float phi_o = atan2f(wp.y, wp.x);
				//store_values_in_buffer(cos_theta_o, phi_o, flux_t, resulting_flux);
				optix_print("(%d) Going out of the medium.\n", i);
				// We are outside
				return true;
			}
		}
	}
	return false;
}