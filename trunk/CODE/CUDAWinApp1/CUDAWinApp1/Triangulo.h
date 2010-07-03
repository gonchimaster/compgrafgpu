#ifndef __TRIANGULO__H__
#define __TRIANGULO__H__

#include <cuda_runtime.h>
#include "tipos.h"




void TrianguloSetVertices(Triangulo* t, float4 v1, float4 v2, float4 v3);
/* Setea los vertices del triangulo. */

#endif