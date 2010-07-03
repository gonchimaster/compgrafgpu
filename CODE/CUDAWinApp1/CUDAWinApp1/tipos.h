#ifndef __TIPOS__H__
#define __TIPOS__H__

#include <cuda_runtime.h>

#define CUDA_ENABLED 1

typedef struct {
	float3 origen;
	float3 dir;
} Rayo;

typedef struct {
	float4 v1;
	float4 v2;
	float4 v3;
} Triangulo;

typedef struct{
	int3 tamanio_grilla;
	int3 resolucion;
	float INFINITO;
	float ZERO;
	int profundidad_recursion;
	//NEW
	int3 threads;
	char nombreEscena[1024];
} Configuracion;



#endif
