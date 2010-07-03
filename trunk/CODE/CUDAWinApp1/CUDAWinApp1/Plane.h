#ifndef __PLANE__H__
#define __PLANE__H__

#include <cuda_runtime.h>

typedef struct {
	float3 punto;
	float3 normal;
} Plane;

#endif