#include <math.h>
#include "Esfera.h"
#include <cuda_runtime.h>
#include <cutil.h>
#include <vector_types.h>
#include <vector_functions.h>
#include <cutil_math.h>

void EsferaSetCentroRadio(Esfera* s, float3 cen, float r){
	s->centro = cen;
	s->radio = r;
}

bool EsferaInterseccionRayo(Rayo r, Esfera e, float* t, float3* point){
	float3 l;
	l = e.centro - r.origen;
	float s = dot(l, r.dir);
	float l2 = dot(l, l);
	if(s < 0 && l2 > powf(e.radio, 2)){
		return false;
	}
	float m2 = l2 - powf(s, 2);
	if(m2 > powf(e.radio, 2)){
		return false;
	}
	float q = sqrtf(powf(e.radio, 2) - m2);
	if(l2 > powf(e.radio, 2)){
		(*t) = s - q;
	}
	else{
		(*t) = s + q;
	}
	(*point) = r.origen + r.dir * (*t);
	return true;
}

void EsferaNormalPuntoInterseccion(Esfera e, float3 puntoSuperficie, float3* normal){
	(*normal) = puntoSuperficie - e.centro;
	(*normal) = normalize((*normal));
}