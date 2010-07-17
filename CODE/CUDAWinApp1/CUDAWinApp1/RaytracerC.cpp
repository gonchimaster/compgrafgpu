#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "Escena.h"
#include "tipos.h"
#include "math.h"
#include "UniformGrid.h"
#include <cuda_runtime.h>
#include <cutil.h>
#include <vector_types.h>
#include <vector_functions.h>
#include <cutil_math.h>

extern Configuracion config;
#ifndef CUDA_ENABLED

bool intersecar(Rayo r, ObjetoEscena obj, float &distancia, float3 & normal){
	if(obj.tipo == Triangle){
		float3 primero = make_float3(obj.tri.v2 - obj.tri.v1);
		float3 segundo = make_float3(obj.tri.v3 - obj.tri.v1);
		normal = cross(primero, segundo);
		normal = normalize(normal);
		float d = -dot(normal, make_float3(obj.tri.v1));
		float t = -(dot(r.origen, normal) + d) / (dot(normal, r.dir));
		if(t < 0) return false;
		float3 I = r.origen + (r.dir * t);
		if(distancia<t) return false;
		float s1 = dot( cross( make_float3(obj.tri.v2 - obj.tri.v1), I - make_float3(obj.tri.v1) ), normal );
		float s2 = dot( cross( make_float3(obj.tri.v3 - obj.tri.v2), I - make_float3(obj.tri.v2) ), normal );
		float s3 = dot( cross( make_float3(obj.tri.v1 - obj.tri.v3), I - make_float3(obj.tri.v3) ), normal );
		
		if((s1 >= 0 && s2 >= 0 && s3 >= 0) || (s1 <= 0 && s2 <= 0 && s3 <= 0)){
			distancia = t;
			normal = ((make_float3(obj.normales.v1) * s2) + (make_float3(obj.normales.v2) * s3)) + (make_float3(obj.normales.v3) * s1);
			return true;
		}
	}
	return false;
}


bool hallarPuntoEntradaGrilla(Rayo r, Escena* es, float3* puntoEntrada){
	
	if(r.origen.x >= es->grilla.bbEscena.minimum.x && r.origen.y >= es->grilla.bbEscena.minimum.y &&
	   r.origen.z >= es->grilla.bbEscena.minimum.z && r.origen.x <= es->grilla.bbEscena.maximum.x &&
	   r.origen.y <= es->grilla.bbEscena.maximum.y && r.origen.z <= es->grilla.bbEscena.maximum.z){
		   (*puntoEntrada) = make_float3(r.origen.x, r.origen.y, r.origen.z);
		   return true;
	}
	
	float min= -config.INFINITO;
	float max = config.INFINITO;
	float3 resta = es->grilla.bbEscena.maximum - es->grilla.bbEscena.minimum;
	resta = resta / 2.0;
	float3 centro = resta + es->grilla.bbEscena.minimum;
	float3 p = centro - r.origen;


	//Plano alineado con X
	float e = p.x;
	float f = r.dir.x;

	float h = resta.x;
	if(abs(f)>=config.ZERO){
		float t1 = (e + h)/f;
		float t2 = (e - h)/f;
		if(t1>t2) {
			float aux = t1;
			t1 = t2;
			t2 = aux;
		}
		min = (t1>min)?t1:min;
		max = (t2<max)?t2:max;
		if(min>max) return false;
		if(max<0) return false;
	}
	else if(-e-h>0 || -e+h<0) return false;


	//Plano alineado con Y
	e = p.y;
	f = r.dir.y;
	h = resta.y;
	if(abs(f)>=config.ZERO){
		float t1 = (e + h)/f;
		float t2 = (e - h)/f;
		if(t1>t2) {
			float aux = t1;
			t1 = t2;
			t2 = aux;
		}
		min = (t1>min)?t1:min;
		max = (t2<max)?t2:max;
		if(min>max) return false;
		if(max<0) return false;
	}
	else if(-e-h>0 || -e+h<0) return false;


	//Plano alineado con Z
	e = p.z;
	f = r.dir.z;

	h = resta.z;
	if(abs(f)>=config.ZERO){
		float t1 = (e + h)/f;
		float t2 = (e - h)/f;
		if(t1>t2) {
			float aux = t1;
			t1 = t2;
			t2 = aux;
		}
		min = (t1>min)?t1:min;
		max = (t2<max)?t2:max;
		if(min>max) return false;
		if(max<0) return false;
	}
	else if(-e-h>0 || -e+h<0) return false;


	if(min>0){
		float3 coordMundo = r.origen + (r.dir * min);
		(*puntoEntrada) = make_float3(coordMundo.x, coordMundo.y, coordMundo.z);
		return true;
	}
	float3 coordMundo = r.origen + (r.dir * max);
	(*puntoEntrada) = make_float3(coordMundo.x, coordMundo.y, coordMundo.z);
	return true;

}


bool intersecarObjetosGrilla(int* listasGrid, int comienzoLista, Rayo r, ObjetoEscena* objetos, float& distancia, float3& normal, int& indiceObjeto){
	bool intersecaron = false;
	float3 normalAux;
	int indiceListaGrid = 0;
	
	indiceObjeto = comienzoLista;
	while(listasGrid[comienzoLista + indiceListaGrid] != -1){
		bool choco = intersecar(r, objetos[listasGrid[comienzoLista + indiceListaGrid]], distancia, normalAux);
		if(choco){
			indiceObjeto = listasGrid[comienzoLista + indiceListaGrid];
			intersecaron = true;
			normal = make_float3(normalAux.x, normalAux.y, normalAux.z);
		}
		indiceListaGrid++;
	}
	return intersecaron;
}

//Calcula los vectores necesarios para la recorrida de los voxels
void calcularInicioGrilla(Rayo r, BoundingBox bbEscena, float3 dimGrid, float3 curPos, float3& incre, float3& tMin){
	float3 voxelSize;
	voxelSize = make_float3(  ((bbEscena.maximum.x - bbEscena.minimum.x) / dimGrid.x),
							  ((bbEscena.maximum.y - bbEscena.minimum.y) / dimGrid.y),
							  ((bbEscena.maximum.z - bbEscena.minimum.z) / dimGrid.z));
	
	incre.x = abs(r.dir.x) > config.ZERO ? voxelSize.x / r.dir.x : 0;
	incre.y = abs(r.dir.y) > config.ZERO ? voxelSize.y / r.dir.y : 0;
	incre.z = abs(r.dir.z) > config.ZERO ? voxelSize.z / r.dir.z : 0;

	if(incre.x == 0)
		tMin.x = config.INFINITO;
	if(incre.y == 0)
		tMin.y = config.INFINITO;
	if(incre.z == 0)
		tMin.z = config.INFINITO;
	
	float3 voxelActual = UniformGridCoordMundoACoordGrid(curPos, bbEscena, dimGrid);
	
	if(r.dir.x > 0)
		voxelActual.x += 1.0f;
	if(r.dir.y > 0)
		voxelActual.y += 1.0f;
	if(r.dir.z > 0)
		voxelActual.z += 1.0f;

	BoundingBox voxelMundo;
	UniformGridVoxelGrillaAVoxelMundo(voxelActual, bbEscena, dimGrid, &voxelMundo);
	
	if(incre.x != 0)
		tMin.x = (voxelMundo.minimum.x - curPos.x) / r.dir.x;
	if(incre.y != 0)
		tMin.y = (voxelMundo.minimum.y - curPos.y) / r.dir.y;
	if(incre.z != 0)
		tMin.z = (voxelMundo.minimum.z - curPos.z) / r.dir.z;
}

//Devuelve false si salio de la grilla y me modifica el voxel actual en el caso de que se 
//pueda avanzar en la grilla
bool siguienteVoxel(float3 &tMin, float3 incre, float3 dimensionGrilla, float3& voxelActual){
	float tMinX = abs(tMin.x);
	float tMinY = abs(tMin.y);
	float tMinZ = abs(tMin.z);
	
	if(tMinX < tMinY){
		if(tMinX < tMinZ){//X min
			tMin.x += abs(incre.x);
			voxelActual.x += incre.x > 0 ? 1.0f : -1.0f;
		}
		else{			  //Z min
			tMin.z += abs(incre.z);
			voxelActual.z += incre.z > 0 ? 1.0f : -1.0f;
		}
	}else{
		if(tMinY < tMinZ){//Y min
			tMin.y += abs(incre.y);
			voxelActual.y += incre.y > 0 ? 1.0f : -1.0f;
		}
		else{			  //Z min
			tMin.z += abs(incre.z);
			voxelActual.z += incre.z > 0 ? 1.0f : -1.0f;
		}
	}

	if(voxelActual.x * voxelActual.x >= dimensionGrilla.x * dimensionGrilla.x ||
	   voxelActual.y * voxelActual.y >= dimensionGrilla.y * dimensionGrilla.y ||
	   voxelActual.z * voxelActual.z >= dimensionGrilla.z * dimensionGrilla.z ||
	   (voxelActual.x < 0 || voxelActual.y < 0 || voxelActual.z < 0)) {
		   return false;
	}
	return true;

}

bool estaEnSombra(Rayo rayoSombra, Escena *escena, int indiceObjetoIntersecado){
	bool sombra = false;
	bool salir_sombra = false;
	float3 entradaSombra, increSombra, tMinSombra, normalS;
	int menorS = 0;
	
	calcularInicioGrilla(rayoSombra, escena->grilla.bbEscena, escena->grilla.dimension, rayoSombra.origen, increSombra, tMinSombra);
	entradaSombra = UniformGridCoordMundoACoordGrid(rayoSombra.origen, escena->grilla.bbEscena, escena->grilla.dimension);
	
	while (!sombra && !salir_sombra){
		int indiceGrillaS = UniformGridVoxelAIndiceListaVoxels(escena->grilla, entradaSombra);
		int comienzoListaS = escena->grilla.voxels[indiceGrillaS];
		float distanciaS = config.INFINITO;
							
		//Interseco con los objetos de la celda de la grilla
		if(comienzoListaS != -1){
			sombra = intersecarObjetosGrilla(escena->grilla.listasGrid, comienzoListaS, rayoSombra, escena->objetos, distanciaS, normalS, menorS);
			sombra = sombra && !(menorS == indiceObjetoIntersecado);
		}
		if(!sombra){
			salir_sombra = !siguienteVoxel(tMinSombra, increSombra, escena->grilla.dimension, entradaSombra);
		}
	}
	return sombra;
}

float3 hallarColor(Rayo r, Escena* escena, int nivel){

	bool salir = false;
	float3 puntoEntada;
	float3 voxelActual;
	float3 intersecMasCercana, V;
	float3 color;
	float3 colorAcumulado;
	color = make_float3(0,0,0);
	colorAcumulado = make_float3(0,0,0);

	bool entra = hallarPuntoEntradaGrilla(r, escena, &puntoEntada);
	voxelActual = UniformGridCoordMundoACoordGrid(puntoEntada, escena->grilla.bbEscena, escena->grilla.dimension);

	while((nivel < config.profundidad_recursion) && !salir && entra){

		bool salirGrilla = false;
		float3 incre;
		float3 tMin;
		float3 normal;
		
		calcularInicioGrilla(r, escena->grilla.bbEscena, escena->grilla.dimension, puntoEntada, incre, tMin);

		bool interseque = false;
		int menor= -1;

		while(!salirGrilla){
			int indiceGrilla = UniformGridVoxelAIndiceListaVoxels(escena->grilla, voxelActual);
			int comienzoLista = escena->grilla.voxels[indiceGrilla];
			menor = comienzoLista;
			bool intersecaron = false;
			float distancia = config.INFINITO;
			
			if(comienzoLista != -1){
				intersecaron = intersecarObjetosGrilla(escena->grilla.listasGrid, comienzoLista, r, escena->objetos, distancia, normal, menor);
				if(intersecaron){
					float3 origen = r.origen + (r.dir * distancia);
					float3 voxInterseccion = UniformGridCoordMundoACoordGrid(origen, escena->grilla.bbEscena, escena->grilla.dimension);
					if((voxelActual.x != voxInterseccion.x) || (voxelActual.y != voxInterseccion.y) || (voxelActual.z != voxInterseccion.z)){
						intersecaron = false;
					}
				}
			}
			
			if(intersecaron){
				//Genero rayo a la luz
				Rayo rayoSombra;
				float3 n;
				bool sombra= false;
				int index = 0;
				
				normal = normalize(normal);

				intersecMasCercana = r.origen + (r.dir * distancia);

				V = intersecMasCercana - r.origen;
				V = normalize(V);

				float3 L = make_float3(escena->luces[0].posicion) - intersecMasCercana;
				float d = length(L);
				rayoSombra.dir = make_float3(L.x, L.y, L.z);
				rayoSombra.dir = normalize(rayoSombra.dir);

				float3 epsilonL = intersecMasCercana + (rayoSombra.dir + 1000000*config.ZERO);
				rayoSombra.origen = make_float3(epsilonL.x, epsilonL.y, epsilonL.z);
								
				sombra = estaEnSombra(rayoSombra, escena, menor);
				if(!sombra){
					L = normalize(L);
					
					//Se calcula la componente difusa...
					float LxN = dot(L, normal);
					if(LxN > 0){
						color = escena->materiales[escena->objetos[menor].id_material].diffuse_color;
						color = color * make_float3(escena->luces[0].color);
						color = color * LxN;
						color = color + escena->materiales[escena->objetos[menor].id_material].ambient_color;
					}
					else{
						color = color + escena->materiales[escena->objetos[menor].id_material].ambient_color;
					}
					
					//Se calcula la componente especular...
					if((escena->materiales[escena->objetos[menor].id_material].specular_color.x != 0) ||
					   (escena->materiales[escena->objetos[menor].id_material].specular_color.y != 0) ||
					   (escena->materiales[escena->objetos[menor].id_material].specular_color.z != 0)){
							float3 H = L + V;
							H = normalize(H);
							float NxH = dot(H, normal);
							if(NxH > 0){
								float3 compSpec = (escena->materiales[escena->objetos[menor].id_material].specular_color *
									powf(NxH, escena->materiales[escena->objetos[menor].id_material].coef_at_especular));
								compSpec = compSpec * escena->materiales[escena->objetos[menor].id_material].diffuse_color;
								color = color + compSpec;
							}
					}
				}
				else {
					color = escena->materiales[escena->objetos[menor].id_material].ambient_color;
				}
				interseque = true;
				salirGrilla=true;
			}

			if(!salirGrilla){
				if(!siguienteVoxel(tMin, incre, escena->grilla.dimension, voxelActual)){
					salirGrilla = true;
					salir = true;
				}
			}


		}/*WHILE NOT SALIR DE LA GRILLA*/

		colorAcumulado = color;

		if(interseque){
			float reflection = escena->materiales[escena->objetos[menor].id_material].reflection;
			float refraction = escena->materiales[escena->objetos[menor].id_material].refraction;
			if(reflection > 0){
				//Armo el rayo de reflexion y avanzo un poco el origen en la direccion del mismo...
				//R = V - 2 * (N.V) * N
				Rayo rayoReflejado;
				float NdotV = dot(normal , V);
				float3 NxNdotV = normal * (2.0f * NdotV);
				rayoReflejado.dir = V - NxNdotV;
				rayoReflejado.dir = normalize(rayoReflejado.dir);
				float3 epsilonR = intersecMasCercana + (rayoReflejado.dir * 1000000*config.ZERO);
				rayoReflejado.origen = epsilonR;
				
				//Hago la llamada recursiva...
				float3 intensidadReflejada = hallarColor(rayoReflejado, escena, nivel + 1);

				//Sumo el color obtenido al color acumulado...
				float3 colorReflexion = intensidadReflejada * reflection;
				colorAcumulado = colorAcumulado * (1 - reflection);
				colorAcumulado = colorAcumulado + colorReflexion;
			}
			else if(refraction > 0){
				//Generar rayo de refraccion...
				Rayo rayoRefractado;
				
				float indiceRefraccionMaterial = escena->materiales[escena->objetos[menor].id_material].index_of_refraction;
				float indiceRefraccionAire = 1.0f; //1.00029f
				float n = indiceRefraccionAire / indiceRefraccionMaterial;
				float cosI = -dot(normal, V);
				int multiplicaNormal = 1;

				if(cosI < 0.0f){
					multiplicaNormal = -1;
					cosI = -dot((normal * multiplicaNormal), V);
					n = indiceRefraccionMaterial;
				}
				float cosT2 = 1.0f - n * n * (1.0f - cosI * cosI);
				if(cosT2 > 0.0){		
					rayoRefractado.dir = ( (normal * multiplicaNormal) * (n * cosI - sqrtf( cosT2 )) + (V * n) );
					rayoRefractado.dir = normalize(rayoRefractado.dir);
					
					float3 epsilonT = intersecMasCercana + (rayoRefractado.dir * 1000000*config.ZERO);
					rayoRefractado.origen = epsilonT;
					
					//Hago la llamada recursiva...
					float3 intensidadTransmitida = hallarColor(rayoRefractado, escena, nivel + 1);

					//Sumo el color obtenido al color acumulado...
				    float3 colorRefraccion = intensidadTransmitida * refraction;
					colorAcumulado = colorAcumulado * (1 - refraction);
					colorAcumulado = colorAcumulado + colorRefraccion;
				}
			}
			else{
				salir = true;
			}
		}
		else{
			salir = true;//TODO???
		}
		salir = true;
	}/*WHILE not PROF && not SALIR*/

	return colorAcumulado;
}

extern "C" char** raytrace(Escena *es){
	char** bitmap = (char**)malloc(config.resolucion.y*sizeof(char*));
	for(int i= 0; i<config.resolucion.y; i++){
		bitmap[i] = (char*)malloc(config.resolucion.x*3*sizeof(char));
	}

	Rayo r;
	r.origen.x = es->camara.ojo.x;
	r.origen.y = es->camara.ojo.y;
	r.origen.z = es->camara.ojo.z;

	float3 dx = make_float3(es->plano_de_vista.v2 - es->plano_de_vista.v1);
	float3 dy = make_float3(es->plano_de_vista.v3 - es->plano_de_vista.v1);
	dx = dx / config.resolucion.x;
	dy = dy / config.resolucion.y;
	float3 inicio_fila;
	float3 iterador;
	
	inicio_fila = make_float3(es->plano_de_vista.v3);
	iterador = make_float3(es->plano_de_vista.v3);
	
	clock_t t_ini, t_fin;
	double secs;

	t_ini = clock();

	for( unsigned y = 0; y < config.resolucion.y; y++) { 
		char *bits = bitmap[config.resolucion.y - 1 -y];
		for( unsigned x = 0; x < config.resolucion.x; x++) {
			//ARMO EL RAYO
			r.dir = iterador - es->camara.ojo;
			r.dir = normalize(r.dir);
			
			//430 - 380
			if(x == 430 && y == (config.resolucion.y - 380))
				printf("");

			float3 color = hallarColor(r, es, 0);

			bits[x*3] = (unsigned)(255 * (color.z > 1.0? 1.0 : color.z));
			bits[x*3+1] = (unsigned)(255 * (color.y > 1.0? 1.0 : color.y));
			bits[x*3+2] = (unsigned)(255 * (color.x > 1.0? 1.0 : color.x));

			iterador = iterador + dx;
		}
		inicio_fila = inicio_fila - dy;
		iterador = inicio_fila;
	}
	t_fin = clock();

	secs = (double)(t_fin - t_ini) / CLOCKS_PER_SEC;
	
	/*printf("\n");
	printf("Triangles In Scene : %d.\n", es->cant_objetos);
	printf("Size Uniform Grid : [%d x %d x %d].\n", config.tamanio_grilla.x, config.tamanio_grilla.y, config.tamanio_grilla.z);
	printf("Execution Time : %.16g milisegundos.\n", secs * 1000.0);
	printf("\n");*/

	return bitmap;
}
#endif
