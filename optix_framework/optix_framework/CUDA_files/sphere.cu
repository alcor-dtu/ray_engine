

#include <device_common.h>
#include <float.h>
#include <material_device.h>

using namespace optix;

rtDeclareVariable(float3, geometric_normal, attribute geometric_normal, );
rtDeclareVariable(float3, shading_normal, attribute shading_normal, );
rtDeclareVariable(float2, texcoord, attribute texcoord, );
//
// (NEW)
// Bounding box program for programmable convex hull primitive
//
rtDeclareVariable(float3, center, , );
rtDeclareVariable(float, radius, , );

RT_PROGRAM void sphere_bounds(int primIdx, float result[6])
{
	optix::Aabb* aabb = (optix::Aabb*)result;
	aabb->m_min = center - make_float3(radius);
	aabb->m_max = center + make_float3(radius);
}

//
// (NEW)
// Intersection program for programmable convex hull primitive
//
RT_PROGRAM void sphere_intersect(int primIdx)
{
	float3 o = ray.origin;
	float3 d = ray.direction;
	float3 l = center - o;
	float s = dot(l, d);
	float l_sq = dot(l, l);
	float rad_sq = radius * radius;

	if (s < 0.0f && l_sq > rad_sq)	 // sphere is behind the ray
		return;

	float m_sq = l_sq - s*s;

	if (m_sq > rad_sq) // sphere in front of the ray but delta < 0 -> no hit
		return;

	float q = sqrt(rad_sq - m_sq);
	float t0 = s - q;
	float t1 = s + q;

	if (rtPotentialIntersection(t0)){
		shading_normal = geometric_normal = normalize(-l + t0*d);
		float3 pos = o + d*t0;
		texcoord = cartesian_to_spherical(pos) / make_float2(M_PIf, 2*M_PIf);
		rtReportIntersection(get_material_index(texcoord));
	}
	else if (rtPotentialIntersection(t1)){
		shading_normal = geometric_normal = normalize(-l + t1*d);
		float3 pos = o + d*t1;
		texcoord = cartesian_to_spherical(pos) / make_float2(M_PIf, 2*M_PIf);
		rtReportIntersection(get_material_index(texcoord));
	}
}
