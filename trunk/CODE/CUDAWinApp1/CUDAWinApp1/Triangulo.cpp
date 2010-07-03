#include "Triangulo.h"

/* Setea los vertices del triangulo. */
void TrianguloSetVertices(Triangulo* t, float4 v1, float4 v2, float4 v3){
	t->v1 = v1;
	t->v2 = v2;
	t->v3 = v3;
}
