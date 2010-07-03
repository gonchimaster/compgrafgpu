#ifndef __SPHERE__H__
#define __SPHERE__H__

#include <cuda_runtime.h>
#include "tipos.h"

typedef struct {
	float3 centro;
	float radio;
} Esfera;

void EsferaSetCentroRadio(Esfera* s, float3 cen, float r);

bool EsferaInterseccionRayo(Rayo r, Esfera e, float* t, float3* point);
/* Evalua la interseccion del rayo r con la esfera e. Retorna TRUE si hay interseccion. */
/* En caso de que haya interseccion se retorna t y point tq point = origen + t*dir. */

void EsferaNormalPuntoInterseccion(Esfera e, float3 puntoSuperficie, float3* normal);
/* Calcula la normal en un punto de la superficie de la esfera. */

#endif