#include "Auxiliar.h"

/* Obtiene el minimo y el maximo entre a, b y c */
void AuxiliarObtenerMinimoMaximo(float a, float b, float c, float* min, float* max){
	(*min) = a;
	(*max) = a;

	if(b < (*min)){
		(*min) = b;
	}
	if(b > (*max)){
		(*max) = b;
	}
	if(c < (*min)){
		(*min) = c;
	}
	if(c > (*max)){
		(*max) = c;
	}
}

/* Retorna la componente especificada 1-X 2-Y 3-Z */
float AuxiliarGetComponente(float3 a, int nroComp){
	switch(nroComp){
		case 1:
			return a.x;
			break; //Creo que no es necesario
		case 2:
			return a.y;
			break; //Creo que no es necesario
		case 3:
			return a.z;
			break; //Creo que no es necesario
	}
	return 0.f;
}

/* Setea con value la componente especificada 1-X 2-Y 3-Z */
void AuxiliarSetComponente(float3* a, int nroComp, float value){
	switch(nroComp){
		case 1:
			a->x = value;
			break;
		case 2:
			a->y = value;
			break;
		case 3:
			a->z = value;
			break;
	}
}