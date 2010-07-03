#ifndef __AUXILIAR__H__
#define __AUXILIAR__H__

#include <cuda_runtime.h>

void AuxiliarObtenerMinimoMaximo(float a, float b, float c, float* min, float* max);
/* Obtiene el minimo y el maximo entre a, b y c */


float AuxiliarGetComponente(float3 a, int nroComp);
/* Retorna la componente especificada 1-X 2-Y 3-Z */


void AuxiliarSetComponente(float3* a, int nroComp, float value);
/* Setea con value la componente especificada 1-X 2-Y 3-Z */

#endif