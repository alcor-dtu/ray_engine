#pragma once
#include <optix_world.h>

#define IMPROVED_ENUM_NAME LightType
#define IMPROVED_ENUM_LIST ENUMITEM(DIRECTIONAL) \
						   ENUMITEM(POINT) \
						   ENUMITEM(SKY) \
						   ENUMITEM(AREA)  
#include "improved_enum.h"

#define IMPROVED_ENUM_NAME RayType
#define IMPROVED_ENUM_LIST ENUMITEM(RADIANCE) \
						   ENUMITEM(SHADOW) \
						   ENUMITEM(DEPTH) \
						   ENUMITEM(ATTRIBUTE)  
#include "improved_enum.h"

#ifdef __CUDACC__
#define DEFAULT(x)
#else
#define DEFAULT(x) = x
#endif

typedef int TexPtr;

template<typename T, int Dim=1>
struct BufPtr
{
    // BufPtr() : buf(-1) {}
    rtBufferId<T, Dim> buf;
    __device__ T& operator*() { return buf[0]; }
    __device__ T* operator->() { return &buf[0]; }
    __device__ int size() { return buf.size(); }
#ifndef __CUDACC__
    __host__ explicit BufPtr(rtBufferId<T, Dim> id) : buf(id) {}
    __host__ explicit BufPtr() : buf(-1) {}
#endif
};

template <typename T>
struct BufPtr<T,1> {
    //__device__ BufPtr() : buf(-1) {}
    rtBufferId<T, 1> buf;
    __device__ T& operator*() { return buf[0]; }
    __device__ T* operator->() { return &buf[0]; }
    __device__ int size() { return buf.size(); }
#ifndef __CUDACC__
    __host__ explicit BufPtr(rtBufferId<T, 1> id) : buf(id) {}
    __host__ explicit BufPtr() : buf(-1) {}
#endif
    __device__ T& operator[](const unsigned int & idx) { return buf[idx]; }
    __device__ T& operator[](const int & idx) { return buf[idx]; }
};

template <typename T>
struct BufPtr<T, 2> {
    //__device__ BufPtr() : buf(-1) {}
    rtBufferId<T, 2> buf;
    __device__ T& operator*() { return buf[0]; }
    __device__ T* operator->() { return &buf[0]; }
    __device__ optix::size_t2 size() { return buf.size(); }
#ifndef __CUDACC__
    __host__ explicit BufPtr(rtBufferId<T, 2> id) : buf(id) {}
    __host__ explicit BufPtr() : buf(-1) {}

#endif
    __device__ T& operator[](const optix::uint2 & idx) { return buf[idx]; }
    __device__ T& operator[](const optix::int2 & idx) { return buf[idx]; }
};

template <typename T>
struct BufPtr<T, 3> {
    //__device__ BufPtr() : buf(-1) {}
    rtBufferId<T, 3> buf;
    __device__ T& operator*() { return buf[0]; }
    __device__ T* operator->() { return &buf[0]; }
    __device__ optix::size_t3 size() { return buf.size(); }
#ifndef __CUDACC__
    __host__ explicit BufPtr(rtBufferId<T, 3> id) : buf(id) {}
    __host__ explicit BufPtr() : buf(-1) {}
#endif
    __device__ T& operator[](const optix::uint3 & idx) { return buf[idx]; }
    __device__ T& operator[](const optix::int3 & idx) { return buf[idx]; }
};