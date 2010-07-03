#ifndef __OBJETOESCENA__H__
#define __OBJETOESCENA__H__

#include <cuda_runtime.h>

typedef enum{
	Triangle,
	Sphere
} TipoObjeto;
#ifndef CUDA_ENABLED
typedef struct {
	float3 diffuse_color; //0..1
	float3 ambient_color; //0..1
	float3 specular_color; //0..1
	float refraction;
	float reflection;
	float transparency;
	int coef_at_especular;
	float index_of_refraction;
} Material;

typedef struct {
	TipoObjeto tipo;
	Triangulo tri;
	Triangulo normales;
	int id_material;
} ObjetoEscena;
#else
//PARA CUDA

typedef struct {
	float4 diffuse_color; //0..1  el w es utilizado para el coef_at_especular
	float4 ambient_color; //0..1
	float4 specular_color; //0..1
	float4 other; //(refraction, reflection, transparency, index_of_refraction)
} Material;

typedef struct {
	TipoObjeto tipo;
	Triangulo tri;
	Triangulo normales;
	int id_material;
} ObjetoEscena;


#endif //CUDA_ENABLED

#endif