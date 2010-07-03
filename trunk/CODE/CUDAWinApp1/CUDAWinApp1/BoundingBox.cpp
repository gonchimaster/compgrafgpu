#include "BoundingBox.h"
#include "Auxiliar.h"
#include <math.h>
#include <cuda_runtime.h>
#include <cutil.h>
#include <vector_types.h>
#include <vector_functions.h>
#include <cutil_math.h>

/*Setea el minimo y el maximo del bounding box los vectores min y max*/
void BoundingBoxSetMinMax(BoundingBox* bb, float3 min, float3 max){
	bb->minimum = min;
	bb->maximum = max;
}


/*Setea el minimo del bounding box los los valores x, y y z*/
void BoundingBoxSetMin(BoundingBox* bb, float x, float y, float z){
	bb->minimum.x = x;
	bb->minimum.y = y;
	bb->minimum.z = z;
}


/*Setea el maximo del bounding box los los valores x, y y z*/
void BoundingBoxSetMax(BoundingBox* bb, float x, float y, float z){
	bb->maximum.x = x;
	bb->maximum.y = y;
	bb->maximum.z = z;
}

/* Calcula el centro del Bounding Box bb */
void BoundingBoxCalculateCenter(BoundingBox bb, float3* centro){

	centro->x = (bb.maximum.x + bb.minimum.x) / 2.0f;
	centro->y = (bb.maximum.y + bb.minimum.y) / 2.0f;
	centro->z = (bb.maximum.z + bb.minimum.z) / 2.0f;
}

void BoundingBoxMerge(BoundingBox bb1, BoundingBox bb2, BoundingBox* bbResult){
	bbResult->minimum.x = bb1.minimum.x < bb2.minimum.x ? bb1.minimum.x : bb2.minimum.x;
	bbResult->minimum.y = bb1.minimum.y < bb2.minimum.y ? bb1.minimum.y : bb2.minimum.y;
	bbResult->minimum.z = bb1.minimum.z < bb2.minimum.z ? bb1.minimum.z : bb2.minimum.z;
	bbResult->maximum.x = bb1.maximum.x > bb2.maximum.x ? bb1.maximum.x : bb2.maximum.x;
	bbResult->maximum.y = bb1.maximum.y > bb2.maximum.y ? bb1.maximum.y : bb2.maximum.y;
	bbResult->maximum.z = bb1.maximum.z > bb2.maximum.z ? bb1.maximum.z : bb2.maximum.z;
}

bool BoundingBoxOverlapPlane(Plane p, BoundingBox bb){

	float3 vMin, vMax;
	float d = -dot(p.normal, p.punto);// Ec. plano. normal * VX + d = 0

	for(int i = 1;i <= 3;i++){
		if(AuxiliarGetComponente(p.normal, i) >= 0){
			AuxiliarSetComponente(&vMin, i, AuxiliarGetComponente(bb.minimum, i));
			AuxiliarSetComponente(&vMax, i, AuxiliarGetComponente(bb.maximum, i));
		}
		else {
			AuxiliarSetComponente(&vMin, i, AuxiliarGetComponente(bb.maximum, i));
			AuxiliarSetComponente(&vMax, i, AuxiliarGetComponente(bb.minimum, i));
		}
	}
	
	if(dot(p.normal, vMin) + d > 0){ //OUTSIDE
		return false;
	}
	if(dot(p.normal, vMax) + d < 0){ //INSIDE
		return false;
	}
	return true;
}


bool BoundingBoxOverlapEsfera(Esfera e, BoundingBox bb){
	float d = 0;

	for(int i = 1;i <= 3;i++){
		if(AuxiliarGetComponente(e.centro, i) < AuxiliarGetComponente(bb.minimum, i)){
			d = d + powf(AuxiliarGetComponente(e.centro, i) - AuxiliarGetComponente(bb.minimum, i), 2);
		}
		else if(AuxiliarGetComponente(e.centro, i) > AuxiliarGetComponente(bb.maximum, i)){
			d = d + powf(AuxiliarGetComponente(e.centro, i) - AuxiliarGetComponente(bb.maximum, i), 2);
		}
	}
	if(d > powf(e.radio, 2)){
		return false;
	}
	return true;
}


bool executeNineTest(Triangulo triTrasladado, float3 e0, float3 e1, float3 e2, float3 boxHalfSize){
	float3 efes [3];
	efes[0] = make_float3(triTrasladado.v2) - make_float3(triTrasladado.v1);
	efes[1] = make_float3(triTrasladado.v3) - make_float3(triTrasladado.v2);
	efes[2] = make_float3(triTrasladado.v1) - make_float3(triTrasladado.v3);

	float3 es [3];
	es[0] = e0;
	es[1] = e1;
	es[2] = e2;

	float3 a_ij;
	float pCero, pUno, pDos;
	for(int i = 0;i < 3;i++){
		for(int j = 0;j < 3;j++){
			a_ij = cross(es[i], efes[j]);
			pCero = dot(a_ij, make_float3(triTrasladado.v1));
			pUno = dot(a_ij, make_float3(triTrasladado.v2));
			pDos = dot(a_ij, make_float3(triTrasladado.v3));
			
			float radius = boxHalfSize.x * abs(a_ij.x) + boxHalfSize.y * abs(a_ij.y) + boxHalfSize.z * abs(a_ij.z);
			float min, max;
			AuxiliarObtenerMinimoMaximo(pCero, pUno, pDos, &min, &max);
			if(min > radius || max < -radius){
				return false;
			}
		}
	}
	return true;
}

bool BoundingBoxOvelapTriangle(Triangulo t, BoundingBox bb){
	
	float3 boxCenter, boxHalfSize;
	float3 e1, e2, e3; /* triangle edges */
	Triangulo triTrasladado;
	
	BoundingBoxCalculateCenter(bb, &boxCenter);
	boxHalfSize = make_float3((bb.maximum.x - bb.minimum.x) / 2.0f, (bb.maximum.y - bb.minimum.y) / 2.0f, (bb.maximum.z - bb.minimum.z) / 2.0f);

	triTrasladado.v1 = make_float4(make_float3(t.v1) - boxCenter);
	triTrasladado.v2 = make_float4(make_float3(t.v2) - boxCenter);
	triTrasladado.v3 = make_float4(make_float3(t.v3) - boxCenter);

	e1 = make_float3(1, 0, 0);
	e2 = make_float3(0, 1, 0);
	e3 = make_float3(0, 0, 1);
	
	//TEST NUMERO 3
	if(!executeNineTest(triTrasladado, e1, e2, e3,boxHalfSize)){
		return false;
	}

	//TEST NUMERO 1
	/* test in X-direction */
	float min, max;
	AuxiliarObtenerMinimoMaximo(triTrasladado.v1.x, triTrasladado.v2.x, triTrasladado.v3.x, &min, &max);
	if(min > boxHalfSize.x || max < -boxHalfSize.x){
		return false;
	}
	/* test in Y-direction */
	AuxiliarObtenerMinimoMaximo(triTrasladado.v1.y, triTrasladado.v2.y, triTrasladado.v3.y, &min, &max);
	if(min > boxHalfSize.y || max < -boxHalfSize.y){
		return false;
	}
	/* test in Z-direction */
	AuxiliarObtenerMinimoMaximo(triTrasladado.v1.z, triTrasladado.v2.z, triTrasladado.v3.z, &min, &max);
	if(min > boxHalfSize.z || max < -boxHalfSize.z){
		return false;
	}
	
	//TEST NUMERO 2
	Plane planoTri;
	planoTri.normal = cross(make_float3(triTrasladado.v2 - triTrasladado.v1), make_float3(triTrasladado.v3 - triTrasladado.v2));
	planoTri.punto = make_float3(triTrasladado.v1);

	BoundingBox bbTrasladado;
	BoundingBoxSetMinMax(&bbTrasladado, bb.minimum - boxCenter, bb.maximum - boxCenter);
	if(!BoundingBoxOverlapPlane(planoTri, bbTrasladado)){
		return false;
	}

	return true;
}

/* Calcula el Bounding Box del triangulo t. */
void BoundingBoxCalcularTriangulo(Triangulo* t, BoundingBox* bb){
	BoundingBoxSetMinMax(bb, make_float3(t->v1), make_float3(t->v1));
	
	float4 * vector = (float4*)t;
	for(int i = 1;i < 3;i++){
		if(vector[i].x < bb->minimum.x){
			bb->minimum.x = vector[i].x;
		} 
		if(vector[i].x > bb->maximum.x){
			bb->maximum.x = vector[i].x;
		}
		if(vector[i].y < bb->minimum.y){
			bb->minimum.y = vector[i].y;
		} 
		if(vector[i].y > bb->maximum.y){
			bb->maximum.y = vector[i].y;
		}
		if(vector[i].z < bb->minimum.z){
			bb->minimum.z = vector[i].z;
		} 
		if(vector[i].z > bb->maximum.z){
			bb->maximum.z = vector[i].z;
		}
	}
}

void BoundingBoxCalcularEsfera(Esfera s, BoundingBox* bb){
	BoundingBoxSetMinMax(bb, s.centro, s.centro);
	float3 radius;
	radius = make_float3(s.radio, s.radio, s.radio);
	bb->minimum = bb->minimum - radius;
	bb->maximum = bb->maximum - radius;
}