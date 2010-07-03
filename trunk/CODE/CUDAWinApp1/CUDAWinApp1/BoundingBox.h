#ifndef __BOUNDINGBOX__H__
#define __BOUNDINGBOX__H__

#include "Triangulo.h"
#include "Plane.h"
#include "Esfera.h"

typedef struct {
	float3 minimum;
	float3 maximum;
} BoundingBox;


void BoundingBoxSetMinMax(BoundingBox* bb, float3 min, float3 max);
/* Setea el minimo y el maximo del bounding box los vectores min y max */

void BoundingBoxSetMin(BoundingBox* bb, float x, float y, float z);
/* Setea el minimo del bounding box los los valores x, y y z */

void BoundingBoxSetMax(BoundingBox* bb, float x, float y, float z);
/* Setea el maximo del bounding box los los valores x, y y z */

void BoundingBoxCalculateCenter(BoundingBox bb, float3* centro);
/* Calcula el centro del Bounding Box bb */

void BoundingBoxMerge(BoundingBox bb1, BoundingBox bb2, BoundingBox* bbResult);
/* Retorna en bbResult un BB que contiene a los BB bb1 y bb2. */

bool BoundingBoxOvelapTriangle(Triangulo t, BoundingBox bb);
/* Retorna TRUE si el triangulo t intersecta al BoundingBox bb */

bool BoundingBoxOverlapPlane(Plane p, BoundingBox bb);
/* Retorna TRUE si el plano p intersecta al BoundingBox bb */

bool BoundingBoxOverlapEsfera(Esfera e, BoundingBox bb);
/* Retorna TRUE si la esfera e intersecta al BoundingBox bb */

void BoundingBoxCalcularTriangulo(Triangulo* t, BoundingBox* bb);
/* Calcula el Bounding Box del triangulo t. */

void BoundingBoxCalcularEsfera(Esfera s, BoundingBox* bb);
/* Calcula el Bonding Box de la esfera s. */

#endif