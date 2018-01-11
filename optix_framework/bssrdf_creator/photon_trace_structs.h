#pragma once
#include "host_device_common.h"
#include "random.h"
#define PHOTON_STATUS_NEW 0
#define PHOTON_STATUS_SCATTERING 1

//#define INCLUDE_SINGLE_SCATTERING/
#define INCLUDE_GEOMETRIC_TERM
#define ACCURATE_RANDOM
#define RAAB_ET_AL_FIX

#ifdef ACCURATE_RANDOM
#define RND_FUNC rnd_accurate
#define SEED_TYPE Seed64
#define PAD_TYPE optix::uint
__device__ __forceinline__ void init_seed(SEED_TYPE & seed, unsigned long long q) { seed.l = q; }
#else
#define RND_FUNC rnd_tea
#define SEED_TYPE optix::uint
#define PAD_TYPE optix::uint2
__device__ __forceinline__ void init_seed(SEED_TYPE & seed, unsigned long long q) { seed = (unsigned int)q; }
#endif

struct PhotonSample
{
	optix::float3 xp; // Position of the photon
	int i;			  // current iteration
	optix::float3 wp; // Direction of the photon
	float flux;		  // Current flux of the photon
	int status;
	SEED_TYPE t;			  // current random seed
	PAD_TYPE pad;
};

__host__ __device__ __forceinline__ PhotonSample get_empty_photon()
{
	PhotonSample p;
	p.xp = optix::make_float3(0);
	p.wp = optix::make_float3(0,0,-1);
	p.i = 0;
	p.flux = 0;
	p.status = PHOTON_STATUS_NEW;
	return p;
}
