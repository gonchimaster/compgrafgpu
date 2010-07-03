#ifndef __UNIFORMGRID__H__
#define __UNIFORMGRID__H__

#include "BoundingBox.h"
#include <cuda_runtime.h>
#include "ObjetoEscena.h"

typedef struct {
	float3 dimension;
	BoundingBox bbEscena;
	int* voxels;
	int* listasGrid;
} UniformGrid;


void UniformGridCrearBruto(ObjetoEscena * escena, int topeEscena, UniformGrid* grilla);
/*Dada la escena, se crea la grilla que particiona la escena en voxles.*/
/*Esta es la forma ineficiente.*/

void UniformGridCrear(ObjetoEscena * escena, int topeEscena, UniformGrid* grilla);
/*Dada la escena, se crea la grilla que particiona la escena en voxles.*/

void UniformGridCrear(Triangulo* listaObjetos, int topeEscena, UniformGrid* grilla);
/*Dada la escena, se crea la grilla que particiona la escena en voxles.*/

float3 UniformGridCoordMundoACoordGrid(float3 coordMundo, BoundingBox bbEscena, float3 dimGrid);
/* Transforma una coordenada de mundo a una coordenada de grilla. */

void UniformGridVoxelGrillaAVoxelMundo(float3 voxelGrilla, BoundingBox bbEscena, float3 dimGrid, BoundingBox* voxelMundo);
/* Dado un voxel en coordenadas de grilla devuelve el mismo voxel en coordenadas de mundo */

int UniformGridVoxelAIndiceListaVoxels(UniformGrid grilla, float3 voxel);
/* Retorna el indice en la lista de voxels del voxel pasado por parametro. */

void UniformGridImprimirEstructura(UniformGrid grilla);
/* Imprime la grilla en la salida estandar. (DEBUG) */

#endif