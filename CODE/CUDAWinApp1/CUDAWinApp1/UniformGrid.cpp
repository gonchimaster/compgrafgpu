#include <stdlib.h>
#include <stdio.h>
#include "UniformGrid.h"
#include "ListaEnteros.h"
#include "tipos.h"
#include <cuda_runtime.h>
#include <cutil.h>
#include <vector_types.h>
#include <vector_functions.h>
#include <cutil_math.h>

extern Configuracion config;

/************* OPERACIONES AUXILIARES *************/

#ifndef CUDA_ENABLED
/* Calcula el Bounding Box de una lista de objetos de tamaño size */
void calculateBoundingBox(ObjetoEscena * objetos, int size, BoundingBox* bBox){
	if(size <= 0)
		return;

	if(objetos[0].tipo == Triangle){
		BoundingBoxCalcularTriangulo(&(objetos[0].tri), bBox);
	}
	else if (objetos[0].tipo == Sphere){
		Esfera e;
		EsferaSetCentroRadio(&e, make_float3(objetos[0].tri.v1), objetos[0].tri.v2.x);
		BoundingBoxCalcularEsfera(e, bBox);
	}
	
	for(int i = 1;i < size;i++){
		BoundingBox bbAux;
		if(objetos[i].tipo == Triangle){
			BoundingBoxCalcularTriangulo(&(objetos[i].tri), &bbAux);
		}
		else if (objetos[i].tipo == Sphere){
			Esfera e;
			EsferaSetCentroRadio(&e, make_float3(objetos[i].tri.v1), objetos[i].tri.v2.x);
			BoundingBoxCalcularEsfera(e, &bbAux);
		}
		BoundingBoxMerge((*bBox), bbAux, bBox);
	}
	float3 uno;
	uno = make_float3(1, 1, 1);
	bBox->minimum = bBox->minimum - uno;
	bBox->maximum = bBox->maximum + uno;
} 

#else
/* Calcula el Bounding Box de una lista de objetos de tamaño size */
void calculateBoundingBox(Triangulo* objetos, int size, BoundingBox* bBox){
	if(size <= 0)
		return;
	
	BoundingBoxCalcularTriangulo(&(objetos[0]), bBox);
	
	for(int i = 1;i < size;i++){
		BoundingBox bbAux;
		BoundingBoxCalcularTriangulo(&(objetos[i]), &bbAux);
		BoundingBoxMerge((*bBox), bbAux, bBox);
	}
	float3 uno;
	uno = make_float3(1, 1, 1);
	bBox->minimum = bBox->minimum - uno;
	bBox->maximum = bBox->maximum + uno;
}



#endif

/* Transforma una coordenada de mundo a una coordenada de grilla. */
float3 UniformGridCoordMundoACoordGrid(float3 coordMundo, BoundingBox bbEscena, float3 dimGrid){
	float3 retorno;
	
	if(bbEscena.maximum.x - bbEscena.minimum.x == 0)
		retorno.x = 0;
	else
		retorno.x = (float)((int)((coordMundo.x - bbEscena.minimum.x) / ((bbEscena.maximum.x - bbEscena.minimum.x) / dimGrid.x)));
	
	if(bbEscena.maximum.y - bbEscena.minimum.y == 0)
		retorno.y = 0;
	else
		retorno.y = (float)((int)((coordMundo.y - bbEscena.minimum.y) / ((bbEscena.maximum.y - bbEscena.minimum.y) / dimGrid.y)));
	
	if(bbEscena.maximum.z - bbEscena.minimum.z == 0)
		retorno.z = 0;
	else
		retorno.z = (float)((int)((coordMundo.z - bbEscena.minimum.z) / ((bbEscena.maximum.z - bbEscena.minimum.z) / dimGrid.z)));

	if(retorno.x == dimGrid.x)
		retorno.x = (float)((int)(dimGrid.x - 1));
	if(retorno.y == dimGrid.y)
		retorno.y = (float)((int)(dimGrid.y - 1));
	if(retorno.z == dimGrid.z)
		retorno.z = (float)((int)(dimGrid.z - 1));

	return retorno;
}

/* Dado un voxel en coordenadas de grilla devuelve el mismo voxel en coordenadas de mundo */
void UniformGridVoxelGrillaAVoxelMundo(float3 voxelGrilla, BoundingBox bbEscena, float3 dimGrid, BoundingBox* voxelMundo){
	int indiceVoxelGrillaX = (int)voxelGrilla.x;
	int indiceVoxelGrillaY = (int)voxelGrilla.y;
	int indiceVoxelGrillaZ = (int)voxelGrilla.z;
	float3 minimumDesdeCeroGrilla, maximumDesdeCeroGrilla;
	
	minimumDesdeCeroGrilla = make_float3(  indiceVoxelGrillaX * ((bbEscena.maximum.x - bbEscena.minimum.x) / dimGrid.x),
										   indiceVoxelGrillaY * ((bbEscena.maximum.y - bbEscena.minimum.y) / dimGrid.y),
										   indiceVoxelGrillaZ * ((bbEscena.maximum.z - bbEscena.minimum.z) / dimGrid.z));
	maximumDesdeCeroGrilla = make_float3(  (indiceVoxelGrillaX + 1) * ((bbEscena.maximum.x - bbEscena.minimum.x) / dimGrid.x),
										   (indiceVoxelGrillaY + 1) * ((bbEscena.maximum.y - bbEscena.minimum.y) / dimGrid.y),
										   (indiceVoxelGrillaZ + 1) * ((bbEscena.maximum.z - bbEscena.minimum.z) / dimGrid.z));

	BoundingBoxSetMinMax(voxelMundo, (bbEscena.minimum + minimumDesdeCeroGrilla),
									 (bbEscena.minimum + maximumDesdeCeroGrilla));
}



#ifndef CUDA_ENABLED
/* Calcula el Bounding Box de un triangulo en las coordenadas de la grilla. */
/* dimGrid: dimension de la grilla de la estructura UG */
void calculateBBObjeto(ObjetoEscena objeto, BoundingBox bbEscena, float3 dimGrid, BoundingBox* bbTriangulo){
	BoundingBox bbCoorMundo;
	if(objeto.tipo == Triangle){
		BoundingBoxCalcularTriangulo(&(objeto.tri), &bbCoorMundo);
	}
	else if (objeto.tipo == Sphere){
		Esfera e;
		EsferaSetCentroRadio(&e, make_float3(objeto.tri.v1), objeto.tri.v2.x);
		BoundingBoxCalcularEsfera(e, &bbCoorMundo);
	}
	
	float3 minimoCoordGrid = UniformGridCoordMundoACoordGrid(bbCoorMundo.minimum, bbEscena, dimGrid);
    float3 maximoCoordGrid = UniformGridCoordMundoACoordGrid(bbCoorMundo.maximum, bbEscena, dimGrid);
	
	BoundingBoxSetMinMax(bbTriangulo, minimoCoordGrid, maximoCoordGrid);
} 
#else
void calculateBBObjeto(Triangulo objeto, BoundingBox bbEscena, float3 dimGrid, BoundingBox* bbTriangulo){
	BoundingBox bbCoorMundo;
	BoundingBoxCalcularTriangulo(&objeto, &bbCoorMundo);
		
	float3 minimoCoordGrid = UniformGridCoordMundoACoordGrid(bbCoorMundo.minimum, bbEscena, dimGrid);
    float3 maximoCoordGrid = UniformGridCoordMundoACoordGrid(bbCoorMundo.maximum, bbEscena, dimGrid);
	
	BoundingBoxSetMinMax(bbTriangulo, minimoCoordGrid, maximoCoordGrid);
}
#endif 

bool menorEnTodo(float3 izq, float3 der){
	return (izq.x <= der.x) && (izq.y <= der.y) && (izq.z <= der.z);
}

/**************************************************/



/*Dada la escena, se crea la grilla que particiona la escena en voxles.*/
/*Esta es la forma ineficiente.*/
#ifndef CUDA_ENABLED
void UniformGridCrearBruto(ObjetoEscena * listaObjetos, int topeEscena, UniformGrid* grilla){
	grilla->dimension = make_float3(config.tamanio_grilla.x, config.tamanio_grilla.y, config.tamanio_grilla.z);
	
	grilla->voxels = (int*) malloc(sizeof(int) * grilla->dimension.x * grilla->dimension.y * grilla->dimension.z);

	for(int i = 0;i < grilla->dimension.x * grilla->dimension.y * grilla->dimension.z;i++){
		grilla->voxels[i] = -1;
	}

	grilla->listasGrid = (int*) malloc(sizeof(int) * 3 * grilla->dimension.x * grilla->dimension.y * grilla->dimension.z);
	
	int topeListasGrid = 0;
	float3 coordActual;
	BoundingBox bbTriangulo;

	calculateBoundingBox(listaObjetos, topeEscena, &(grilla->bbEscena));

	for(int k = 0;k < grilla->dimension.z;k++){
		coordActual.z = k;
		for(int j = 0; j < grilla->dimension.y; j++){
			coordActual.y = j;
			for(int i = 0; i < grilla->dimension.x;i++){
				coordActual.x = i;
				for(int tris = 0; tris < topeEscena;tris++){
					
					calculateBBObjeto(listaObjetos[tris] ,grilla->bbEscena, grilla->dimension, &bbTriangulo);
					
					
					if(menorEnTodo(bbTriangulo.minimum, coordActual) && menorEnTodo(coordActual, bbTriangulo.maximum)){
						grilla->listasGrid[topeListasGrid] = tris;
						int indiceGrilla = (k * grilla->dimension.y * grilla->dimension.x) + (j * grilla->dimension.x) + i;
						if(grilla->voxels[indiceGrilla] == -1){
							grilla->voxels[indiceGrilla] = topeListasGrid;
						}
						topeListasGrid++;
					}
				}
				grilla->listasGrid[topeListasGrid] = -1;
				topeListasGrid++;
			}
		}
	}
}
#endif



#ifndef CUDA_ENABLED
/*Dada la escena, se crea la grilla que particiona la escena en voxles.*/
void UniformGridCrear(ObjetoEscena * listaObjetos, int topeEscena, UniformGrid* grilla){
	
	grilla->dimension = make_float3(config.tamanio_grilla.x, config.tamanio_grilla.y, config.tamanio_grilla.z);
	
	grilla->voxels = (int*) malloc(sizeof(int) * (int)(grilla->dimension.x * grilla->dimension.y * grilla->dimension.z));

	calculateBoundingBox(listaObjetos, topeEscena, &(grilla->bbEscena));

	ListaEnteros* listasIdTriangulos = (ListaEnteros*)malloc(sizeof(ListaEnteros) * (int)(grilla->dimension.x * grilla->dimension.y * grilla->dimension.z));
	for(int i = 0;i < (int)(grilla->dimension.x * grilla->dimension.y * grilla->dimension.z);i++){
		listasIdTriangulos[i] = NULL;
	}

	int topeListasGrid = 0;
	int sizeListasGrid = 0; //Entero para calcular el tamaño de listasGrid
	float3 voxelGrilla;
	BoundingBox voxelMundo;
	BoundingBox bbTriangulo;
	for(int i = 0;i < topeEscena;i++){
		calculateBBObjeto(listaObjetos[i] ,grilla->bbEscena, grilla->dimension, &bbTriangulo);
		
		for(int indiceZ = (int)bbTriangulo.minimum.z;indiceZ <= (int)bbTriangulo.maximum.z;indiceZ++){
			for(int indiceY = (int)bbTriangulo.minimum.y;indiceY <= (int)bbTriangulo.maximum.y;indiceY++){
				for(int indiceX = (int)bbTriangulo.minimum.x;indiceX <= (int)bbTriangulo.maximum.x;indiceX++){
					voxelGrilla = make_float3(indiceX, indiceY, indiceZ);
					UniformGridVoxelGrillaAVoxelMundo(voxelGrilla, grilla->bbEscena, grilla->dimension, &voxelMundo);
					
					bool bbOverlapObjeto;
					if(listaObjetos[i].tipo == Triangle){
						bbOverlapObjeto = BoundingBoxOvelapTriangle(listaObjetos[i].tri, voxelMundo);
					}
					else if(listaObjetos[i].tipo == Sphere){
						Esfera e;
						EsferaSetCentroRadio(&e, make_float3(listaObjetos[i].tri.v1), listaObjetos[i].tri.v2.x);
						bbOverlapObjeto = BoundingBoxOverlapEsfera(e, voxelMundo);
					}

					if(bbOverlapObjeto){
						int indiceGrilla = (indiceZ * grilla->dimension.y * grilla->dimension.x) + (indiceY * grilla->dimension.x) + indiceX;
						if(listasIdTriangulos[indiceGrilla] == NULL){
							listasIdTriangulos[indiceGrilla] = ListaEnterosCreate();
							sizeListasGrid++; //Por cada lista se tiene que agregar un -1 en grilla->listasGrid
						}
						listasIdTriangulos[indiceGrilla] = ListaEnterosInsert(listasIdTriangulos[indiceGrilla], i);
						sizeListasGrid++; //Por cada referencia a triangulo se agrega un elemento a grilla->listasGrid
					}
				}
			}
		}
	}

	grilla->listasGrid = (int*) malloc(sizeof(int) * sizeListasGrid);

	for(int i = 0;i < (grilla->dimension.x * grilla->dimension.y * grilla->dimension.z);i++){
		if(listasIdTriangulos[i] != NULL){
			grilla->voxels[i] = topeListasGrid;
			
			ListaEnteros aux = listasIdTriangulos[i];
			while(!ListaEnterosIsEmpty(aux)){
				grilla->listasGrid[topeListasGrid] = ListaEnterosHead(aux);
				topeListasGrid++;
				aux = ListaEnterosTail(aux);
			}
			grilla->listasGrid[topeListasGrid] = -1;
			topeListasGrid++;

			ListaEnterosDispose(listasIdTriangulos[i]);
		}
		else{
			grilla->voxels[i] = -1;
		}
	}
	free(listasIdTriangulos);
}


#else
/*Dada la escena, se crea la grilla que particiona la escena en voxles.*/
void UniformGridCrear(Triangulo* listaObjetos, int topeEscena, UniformGrid* grilla){
	
	grilla->dimension = make_float3((float)(config.tamanio_grilla.x), (float)(config.tamanio_grilla.y), (float)(config.tamanio_grilla.z));
	
	grilla->voxels = (int*) malloc(sizeof(int) * (int)(grilla->dimension.x * grilla->dimension.y * grilla->dimension.z));

	calculateBoundingBox(listaObjetos, topeEscena, &(grilla->bbEscena));

	ListaEnteros* listasIdTriangulos = (ListaEnteros*)malloc(sizeof(ListaEnteros) * (int)(grilla->dimension.x * grilla->dimension.y * grilla->dimension.z));
	for(int i = 0;i < (int)(grilla->dimension.x * grilla->dimension.y * grilla->dimension.z);i++){
		listasIdTriangulos[i] = NULL;
	}

	int topeListasGrid = 0;
	int sizeListasGrid = 0; //Entero para calcular el tamaño de listasGrid
	float3 voxelGrilla;
	BoundingBox voxelMundo;
	BoundingBox bbTriangulo;
	for(int i = 0;i < topeEscena;i++){
		calculateBBObjeto(listaObjetos[i] ,grilla->bbEscena, grilla->dimension, &bbTriangulo);
		
		for(int indiceZ = (int)bbTriangulo.minimum.z;indiceZ <= (int)bbTriangulo.maximum.z;indiceZ++){
			for(int indiceY = (int)bbTriangulo.minimum.y;indiceY <= (int)bbTriangulo.maximum.y;indiceY++){
				for(int indiceX = (int)bbTriangulo.minimum.x;indiceX <= (int)bbTriangulo.maximum.x;indiceX++){
					voxelGrilla = make_float3((float)indiceX, (float)indiceY, (float)indiceZ);
					UniformGridVoxelGrillaAVoxelMundo(voxelGrilla, grilla->bbEscena, grilla->dimension, &voxelMundo);
					
					bool bbOverlapObjeto;
					bbOverlapObjeto = BoundingBoxOvelapTriangle(listaObjetos[i], voxelMundo);

					if(bbOverlapObjeto){
						int indiceGrilla = (indiceZ * (int)grilla->dimension.y * (int)grilla->dimension.x) + (indiceY * (int)grilla->dimension.x) + indiceX;
						if(listasIdTriangulos[indiceGrilla] == NULL){
							listasIdTriangulos[indiceGrilla] = ListaEnterosCreate();
							sizeListasGrid++; //Por cada lista se tiene que agregar un -1 en grilla->listasGrid
						}
						listasIdTriangulos[indiceGrilla] = ListaEnterosInsert(listasIdTriangulos[indiceGrilla], i);
						sizeListasGrid++; //Por cada referencia a triangulo se agrega un elemento a grilla->listasGrid
					}
				}
			}
		}
	}

	grilla->listasGrid = (int*) malloc(sizeof(int) * sizeListasGrid);

	for(int i = 0;i < (grilla->dimension.x * grilla->dimension.y * grilla->dimension.z);i++){
		if(listasIdTriangulos[i] != NULL){
			grilla->voxels[i] = topeListasGrid;
			
			ListaEnteros aux = listasIdTriangulos[i];
			while(!ListaEnterosIsEmpty(aux)){
				grilla->listasGrid[topeListasGrid] = ListaEnterosHead(aux);
				topeListasGrid++;
				aux = ListaEnterosTail(aux);
			}
			grilla->listasGrid[topeListasGrid] = -1;
			topeListasGrid++;

			ListaEnterosDispose(listasIdTriangulos[i]);
		}
		else{
			grilla->voxels[i] = -1;
		}
	}
	free(listasIdTriangulos);
}


#endif //CUDA_ENABLED
/* Imprime la grilla en la salida estandar. (DEBUG) */
void UniformGridImprimirEstructura(UniformGrid grilla){
	for(int i = 0;i < grilla.dimension.x * grilla.dimension.y * grilla.dimension.z;i++){
		printf("Indice voxel %d: ", i);

		int indiceListaVoxel = grilla.voxels[i];
		if(indiceListaVoxel == -1){
			printf("%s", "vacio");
		}
		else{
			while(grilla.listasGrid[indiceListaVoxel] != -1){
				printf("%d ", grilla.listasGrid[indiceListaVoxel]);
				indiceListaVoxel++;
			}
		}
		printf("\n");
	}
}

/* Retorna el indice en la lista de voxels del voxel pasado por parametro. */
int UniformGridVoxelAIndiceListaVoxels(UniformGrid grilla, float3 voxel){
	int indice = 0;

	indice += (int)(voxel.z * grilla.dimension.y * grilla.dimension.x);
	indice += (int)(voxel.y * grilla.dimension.x);
	indice += (int)(voxel.x);

	return indice;
}

