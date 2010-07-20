#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h>
#include <cutil.h>
#include "Escena.h"
#include "tipos.h"
#include "math.h"

#include <vector_types.h>
#include <vector_functions.h>
#include <math_functions.h>
#include <cutil_math.h>


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////          DECLARACIONES           //////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////


//Declaración de las texturas que contendrán la estructura de la aplicación

texture<float4, 1, cudaReadModeElementType> textura_triangulos;//Objetos de la escena
texture<float4, 1, cudaReadModeElementType> textura_normales;//Normales de la escena
texture<float4, 1, cudaReadModeElementType> textura_luces;//Luces
texture<int, 1, cudaReadModeElementType> textura_voxels;//Lista de indices de la lista de objetos
texture<int, 1, cudaReadModeElementType> textura_listasGrid;//Lista de elementos que se encuentran dentro de un voxel particular.
texture<float4, 1, cudaReadModeElementType> textura_rayos;//Rayos a trazar.
texture<float4, 1, cudaReadModeElementType> textura_materiales;//Materiales para todos los objetos de la escena.


//VARIABLES DEFINIDAS A NIVEL DE MEMORIA CONSTANTE EN LA GPU
__constant__ Configuracion configuracion_gpu;//Parámetros de configuración
__constant__ float3 ojo;
__constant__ float3 dx;
__constant__ float3 dy;
__constant__ float3 ini;
__constant__ float cant_luces;
__constant__ float3 dimension_grilla;
__constant__ BoundingBox bounding_box;
__constant__ float3 tam_voxel;
__constant__ float3 tam_grilla;


//Variables utilizadas...
float3* d_color;
float4* d_listaRayos;


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////       FUNCIONES EN GPU           //////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

//#define OLD_ALGORITHM

__device__ bool intersecar(Rayo r, float &distancia, float3 & normal, int ind_obj, bool calc_normal){
	float3 v1 = make_float3(tex1Dfetch(textura_triangulos, 3 * ind_obj));
	float3 v2 = make_float3(tex1Dfetch(textura_triangulos, 3 * ind_obj + 1));
	float3 v3 = make_float3(tex1Dfetch(textura_triangulos, 3 * ind_obj + 2));
	
#ifndef OLD_ALGORITHM
	float3 lado1 = v2-v1;
	float3 lado2 = v3-v1;

	float3 p= cross(r.dir,lado2);
	float determinante = dot(lado1, p);

	if(determinante > -0.001f && determinante < 0.001f)
		return false;

	float inv_det = 1.0f/determinante;
	float3 t = ( r.origen-v1 );

	float u = dot(t,p)* inv_det;
	if(u<0.0f || u>1.0f)
		return false;

	float3 q = cross(t, lado1);

	float v = dot (r.dir, q) * inv_det;
	if(v<0.0f || u+v>1.0f)
		return false;

	float dist = dot(lado2, q)*inv_det;
	if(dist < 0)
		return false;
	
	if(dist>distancia)
		return false;

	distancia = dist;
	if(calc_normal){
		float s = 1-u-v;
		float3 normal_v1 = make_float3(tex1Dfetch(textura_normales, 3 * ind_obj));
		float3 normal_v2 = make_float3(tex1Dfetch(textura_normales, 3 * ind_obj + 1));
		float3 normal_v3 = make_float3(tex1Dfetch(textura_normales, 3 * ind_obj + 2));
		normal = (normal_v1 * s + normal_v2 * u + normal_v3 * v);
	}
	return true;
#else	

	float3 primero = v2 - v1;
	float3 segundo = v3 - v1;
	normal = normalize(cross(primero,segundo));
		
	float d = -dot(normal, v1);
	float t = -( dot(r.origen, normal) + d ) / ( dot(normal,r.dir) );
	if(t < 0.001){
		return false;
	}
	float3 I = r.origen + r.dir * t;
	if(distancia < t){
		return false;
	}
	float s1 = dot( cross(primero,(I - v1)), normal );
	float s2 = dot( cross((v3 - v2),(I - v2)), normal );
	float s3 = dot( cross((-segundo),(I - v3)), normal );
	if((s1 >= 0 && s2 >= 0 && s3 >= 0) || (s1 <= 0 && s2 <= 0 && s3 <= 0)){
		distancia = t;
		if(calc_normal){
			float3 normal_v1 = make_float3(tex1Dfetch(textura_normales, 3 * ind_obj));
			float3 normal_v2 = make_float3(tex1Dfetch(textura_normales, 3 * ind_obj + 1));
			float3 normal_v3 = make_float3(tex1Dfetch(textura_normales, 3 * ind_obj + 2));
			normal = normal_v1 * s2 + normal_v2 * s3 + normal_v3 * s1;
		}
		return true;
	}
	return false;
#endif
}


__device__ float3 coordMundoACoordGrid(float3 coordMundo){
	float3 retorno;
	if(bounding_box.minimum.x==bounding_box.maximum.x){
		retorno.x = 0.f;
	}else{
		retorno.x =(int)(((coordMundo.x-bounding_box.minimum.x)/(bounding_box.maximum.x-bounding_box.minimum.x))*(dimension_grilla.x));
	}

	if(bounding_box.minimum.y==bounding_box.maximum.y){
		retorno.y = 0.f;
	}else{
		retorno.y =(int)(((coordMundo.y-bounding_box.minimum.y)/(bounding_box.maximum.y-bounding_box.minimum.y))*(dimension_grilla.y));
	}

	if(bounding_box.minimum.z==bounding_box.maximum.z){
		retorno.z = 0.f;
	}else{
		retorno.z =(int)(((coordMundo.z-bounding_box.minimum.z)/(bounding_box.maximum.z-bounding_box.minimum.z))*(dimension_grilla.z));
	}

	if(retorno.x==dimension_grilla.x){
		retorno.x = (int)(dimension_grilla.x-1);
	}
	if(retorno.y==dimension_grilla.y){
		retorno.y = (int)(dimension_grilla.y-1);
	}
	if(retorno.z==dimension_grilla.z){
		retorno.z = (int)(dimension_grilla.z-1);
	}
	return retorno;
}


__device__ float3 coordGridACoordMundo(float3 voxelActual, float3* voxelMundo){
	*voxelMundo = bounding_box.minimum + voxelActual*tam_voxel;
	return *voxelMundo;
}

__device__ bool hallarPuntoEntradaGrilla(Rayo r, float3* puntoEntrada){
	if(r.origen.x >= bounding_box.minimum.x && r.origen.y >= bounding_box.minimum.y &&
	   r.origen.z >= bounding_box.minimum.z && r.origen.x <= bounding_box.maximum.x &&
	   r.origen.y <= bounding_box.maximum.y && r.origen.z <= bounding_box.maximum.z){
		   (*puntoEntrada) = make_float3(r.origen.x, r.origen.y, r.origen.z);
		   return true;
	}

	float min= -configuracion_gpu.INFINITO;
	float max = configuracion_gpu.INFINITO;

	float3 resta = tam_grilla;
	float3 centro = resta + bounding_box.minimum;
	float3 p = centro - r.origen;

	//Plano alineado con X
	float e = p.x;
	float f = r.dir.x;

	float h = resta.x;
	if(abs(f)>=configuracion_gpu.ZERO){
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
	if(abs(f)>=configuracion_gpu.ZERO){
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
	if(abs(f)>=configuracion_gpu.ZERO){
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
		*puntoEntrada = r.origen + r.dir * min;
		return true;
	}
	*puntoEntrada = r.origen + r.dir * max;
	return true;

}



__device__ bool intersecarObjetosGrilla(int comienzoLista, Rayo r, float& distancia, float3& normal, int& indiceObjeto, bool calc_normal){
	
	if (comienzoLista == -1) 
		return false;

	bool intersecaron = false;
	indiceObjeto = comienzoLista;
	float3 normalAux;

	int valor = tex1Dfetch(textura_listasGrid, comienzoLista);
	while(valor != -1){
		bool choco = intersecar(r, distancia, normalAux, valor, calc_normal);
		if(choco){
			indiceObjeto = valor;
			if(calc_normal)
				normal = normalAux;
			intersecaron= true;
		}
		comienzoLista++;
		valor = tex1Dfetch(textura_listasGrid, comienzoLista);
	}
	return intersecaron;
}

//Calcula los vectores necesarios para la recorrida de los voxels
__device__ void calcularInicioGrilla(float3 dir, float3 curPos, float3& incre, float3& tMin){
	
	float3 voxelSize = tam_voxel;

	incre.x = abs(dir.x)> configuracion_gpu.ZERO ? (voxelSize.x)/dir.x:0;
	incre.y = abs(dir.y)> configuracion_gpu.ZERO ? (voxelSize.y)/dir.y:0;
	incre.z = abs(dir.z)> configuracion_gpu.ZERO ? (voxelSize.z)/dir.z:0;

	if(incre.x == 0)
		tMin.x = configuracion_gpu.INFINITO;
	if(incre.y == 0)
		tMin.y = configuracion_gpu.INFINITO;
	if(incre.z == 0)
		tMin.z = configuracion_gpu.INFINITO;

	float3 voxelActual;
	voxelActual = coordMundoACoordGrid(curPos);

	if(dir.x>0)
		voxelActual.x+=1.0f;
	if(dir.y>0)
		voxelActual.y+=1.0f;
	if(dir.z>0)
		voxelActual.z+=1.0f;

	float3 voxelMundo;
	coordGridACoordMundo(voxelActual,&voxelMundo);

	if(incre.x != 0)
		tMin.x = (voxelMundo.x - curPos.x)/dir.x;
	if(incre.y != 0)
		tMin.y = (voxelMundo.y - curPos.y)/dir.y;
	if(incre.z != 0)
		tMin.z = (voxelMundo.z - curPos.z)/dir.z;

}


//Devuelve false si salio de la grilla y me modifica el voxel actual en el caso de que se 
//pueda avanzar en la grilla
__device__ bool siguienteVoxel(float3 &tMin, float increX, float increY, float increZ,  float3& voxelActual){
	float tMinx = abs(tMin.x);
	float tMiny = abs(tMin.y);
	float tMinz = abs(tMin.z);
	if(tMinx<tMiny){
		if(tMinx<tMinz){ //X min
			tMin.x+=abs(increX);
			voxelActual.x+=increX>0?1.0f:-1.0f;
		}
		else{			 //Z min
			tMin.z+=abs(increZ);
			voxelActual.z+=increZ>0?1.0f:-1.0f;
		}
	}else{
		if(tMiny<tMinz){ //Y min
			tMin.y+=abs(increY);
			voxelActual.y+=increY>0?1.0f:-1.0f;
		}
		else{			 //Z min
			tMin.z+=abs(increZ);
			voxelActual.z+=increZ>0?1.0f:-1.0f;
		}
	}

	if(voxelActual.x*voxelActual.x>=dimension_grilla.x*dimension_grilla.x||
		voxelActual.y*voxelActual.y>=dimension_grilla.y*dimension_grilla.y||
		voxelActual.z*voxelActual.z>=dimension_grilla.z*dimension_grilla.z||
		voxelActual.x<0||voxelActual.y<0||voxelActual.z<0){
		return false;
	}
	return true;

}

////<<<<<<<<<<<<< HALLAR RAYOS >>>>>>>>>>>>>////
__global__ void hallarColor(float3* retorno){

		int indiceR = configuracion_gpu.resolucion.x*(blockIdx.y*blockDim.y+threadIdx.y)+(blockIdx.x*blockDim.x+threadIdx.x);

		//Rayo actual que me permite ejecutar la iteración.
		Rayo rayoactual;
		rayoactual.dir = make_float3(tex1Dfetch(textura_rayos, indiceR));
		rayoactual.origen = ojo;
	
		int nivel = 0;
		bool salir = false;
		float3 voxelActual;
		float3 puntoEntrada;

		//Hallo el punto de entrada a la grilla y calculo el voxel que corresponde.
		bool entra = hallarPuntoEntradaGrilla(rayoactual, &puntoEntrada);
		voxelActual = coordMundoACoordGrid(puntoEntrada);
		
		float3 color = make_float3(0,0,0);
		float3 colorAcumulado = make_float3(0,0,0);
		
		//TODO ADD
		float multiplicador = 1.0;
		//float3 normal;


		// Para la refracción
		bool adentro = false;

		float3 normal = make_float3(0,0,0);
		while((nivel < configuracion_gpu.profundidad_recursion) && !salir && entra){
			
			bool salirGrilla = false;
			
			//Extrañamente si no inicializo esto no funciona... NO COMPILA!!! cuack
			float3 tMin = make_float3(0,0,0);
			float3 incre = make_float3(0,0,0);

			//Calcula los parámetros necesarios para recorrer la grilla
			calcularInicioGrilla(rayoactual.dir, puntoEntrada, incre, tMin);

			
			bool interseque = false;
			int menor=-1;
			float distancia;
	
			while(!salirGrilla){
				int indiceGrilla = (voxelActual.z * dimension_grilla.y* dimension_grilla.x) + (voxelActual.y * dimension_grilla.x) + voxelActual.x;
				int comienzoLista = tex1Dfetch(textura_voxels, indiceGrilla);
				menor= comienzoLista;
				bool intersecaron = false;
				distancia = configuracion_gpu.INFINITO;
				
				//Interseco con los objetos de la celda de la grilla
				if(comienzoLista!=-1){
					intersecaron = intersecarObjetosGrilla(comienzoLista, rayoactual, distancia, normal, menor, true);
					normal = normalize(normal);
					if(intersecaron){
						float3 prod = rayoactual.dir * distancia;
						float3 origen;
						origen = rayoactual.origen + prod;
						origen = coordMundoACoordGrid(origen);
						if(!((origen.x == voxelActual.x) && (origen.y == voxelActual.y) && (origen.z == voxelActual.z))){
							intersecaron = false;
						}
					}
				}

				if(intersecaron){					
					//Genero rayo a la luz
					Rayo rayoSombra;
					//CALCULO LA SOMBRA
					float sombra = 0.0;
					float3 origen = rayoactual.origen + (distancia * rayoactual.dir);
					
					float3 dirSombra;

					//TODO MAS LUCES
					//Para todas las luces
					int ind_luces = 0;
					int id_material = tex1Dfetch(textura_triangulos, 3 * menor).w;
					float3 colorDif = make_float3(tex1Dfetch(textura_materiales,4* id_material));
					float3 colorAmb = make_float3(tex1Dfetch(textura_materiales, 1+4*id_material));
					color = colorAmb;
		
					while((ind_luces < (int)cant_luces)){
						dirSombra = make_float3(tex1Dfetch(textura_luces, ind_luces*2)) - origen;
						rayoSombra.dir = normalize(dirSombra);
						rayoSombra.origen = origen + rayoSombra.dir * 1000000 * configuracion_gpu.ZERO;

						bool salir_sombra = false;
						float3 entradaSombra= make_float3(0,0,0);
						float3 increSombra= make_float3(0,0,0);
						float3 tMinSombra = make_float3(0,0,0);
						float3 normalS= make_float3(0,0,0);
						int menorS=0;					
						
						calcularInicioGrilla(rayoSombra.dir,rayoSombra.origen, increSombra, tMinSombra);

						entradaSombra = coordMundoACoordGrid(rayoSombra.origen);
						sombra = 0.f;
						
						while (sombra<1.0f && !salir_sombra){
							int indiceGrillaS = (entradaSombra.z * dimension_grilla.y* dimension_grilla.x) + (entradaSombra.y * dimension_grilla.x) + entradaSombra.x;

							int comienzoListaS = tex1Dfetch(textura_voxels, indiceGrillaS);
							float distanciaS = configuracion_gpu.INFINITO;						
							
							//Interseco con los objetos de la celda de la grilla
							if(comienzoListaS!=-1){
								bool mas_sombra = intersecarObjetosGrilla(comienzoListaS, rayoSombra, distanciaS, normalS, menorS, false);
								if(mas_sombra){
									//TODO calcular incremento de la sombra
									sombra=1.0;
								}
							}
							if(sombra<1.0){
								salir_sombra = !siguienteVoxel(tMinSombra, increSombra.x, increSombra.y, increSombra.z, entradaSombra);
							}
						}
						if( sombra < 1.0 )
						{
							float LxN = dot(rayoSombra.dir,normal);
							if (LxN>0)
							{
								float3 luz = make_float3(tex1Dfetch(textura_luces, ind_luces*2+1));
								color+= colorDif * luz * LxN;
							}
						}
						ind_luces++;
					}

					interseque = true;
				    salirGrilla = true;
				}
	
				if(!salirGrilla){
					if(!siguienteVoxel(tMin, incre.x, incre.y, incre.z, voxelActual)){
						salirGrilla = true;
						salir = true;
					}
				}
				
	
			}//WHILE NOT SALIR DE LA GRILLA
			
			if(interseque && nivel<configuracion_gpu.profundidad_recursion){
				int indice_material = (int)(tex1Dfetch(textura_triangulos, 3 * menor).w);
				float4 other = tex1Dfetch(textura_materiales, 3 + 4 * indice_material);
				float indiceEspecular = other.x + other.y;
				colorAcumulado = colorAcumulado + color * multiplicador * (1 - indiceEspecular);
				multiplicador *= indiceEspecular;
				if(other.x > 0){
					nivel++;
					
					float refraccion = adentro ? other.w : 1/other.w;

					float3 normal_refra = adentro ? -normal : normal;
					double cosI = -dot(normal_refra, rayoactual.dir);
					double cosT_cuadrado = 1.0f - refraccion * refraccion * (1.0f - cosI * cosI);

					if (cosT_cuadrado > 0) { //no ocurre la reflexión interna total
						 rayoactual.origen = (rayoactual.origen + (rayoactual.dir* distancia));
						 rayoactual.dir = normalize((rayoactual.dir * refraccion) + (normal_refra*(refraccion * cosI - sqrtf( cosT_cuadrado ))));
						 rayoactual.origen = rayoactual.origen + (rayoactual.dir * 10000 * configuracion_gpu.ZERO);
						 puntoEntrada = rayoactual.origen;
						 color = make_float3(0,0,0);
					}

					adentro = !adentro;

				}
				else if(other.y>0){
					nivel++;
					rayoactual.origen = (rayoactual.dir * (distancia)) + rayoactual.origen;
					//Calculo el rayo para la reflexion

					float VxN = dot(rayoactual.dir, normal) * 2.0f;
					rayoactual.dir = normalize(rayoactual.dir - normal * VxN);
					rayoactual.origen = rayoactual.origen+ normal * 100000*configuracion_gpu.ZERO;
					puntoEntrada = rayoactual.origen;
					color = make_float3(0,0,0);
				}
				else{
					salir = true;
				}
			}
			else{
				salir = true;//TODO???
				colorAcumulado = make_float3(0.5f,0.5f,1.0f);
			}
		}//WHILE not PROF && not SALIR
	
		if(!entra){
			colorAcumulado = make_float3(0.5f,0.5f,1.0f);
		}
		int indice = configuracion_gpu.resolucion.x*(blockIdx.y*blockDim.y+threadIdx.y)+(blockIdx.x*blockDim.x+threadIdx.x);
		retorno[indice] = colorAcumulado;
}


////<<<<<<<<<<<<< CALCULAR RAYOS >>>>>>>>>>>>>////
__global__ void calcularRayos(float4* rayos){
	int indiceRayo = configuracion_gpu.resolucion.x*(blockIdx.y*blockDim.y+threadIdx.y)+(blockIdx.x*blockDim.x+threadIdx.x);
	rayos[indiceRayo] = make_float4((ini - dy*(blockIdx.y*blockDim.y+threadIdx.y)+dx*(blockIdx.x*blockDim.x+threadIdx.x))-ojo,0);
	rayos[indiceRayo] = normalize(rayos[indiceRayo]);
}


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////    FUNCIONES PARA INVOCAR DE C   //////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#ifdef CUDA_ENABLED

extern "C" void UpdateCamera(Escena* es, Configuracion conf){

	//SE ACTUALIZA LA CAMARA
	CUDA_SAFE_CALL(cudaMemcpyToSymbol(ojo,&(es->camara.ojo),sizeof(float3)));
	float3 dx_aca = make_float3((es->plano_de_vista.v2 - es->plano_de_vista.v1)/(float)conf.resolucion.x);
	float3 dy_aca = make_float3((es->plano_de_vista.v3 - es->plano_de_vista.v1)/(float)conf.resolucion.y);
	CUDA_SAFE_CALL(cudaMemcpyToSymbol(dx,&(dx_aca),sizeof(float3)));
	CUDA_SAFE_CALL(cudaMemcpyToSymbol(dy,&(dy_aca),sizeof(float3)));
	CUDA_SAFE_CALL(cudaMemcpyToSymbol(ini,&(es->plano_de_vista.v3),sizeof(float3)));

	//SE RE-CALCULAN LOS RAYOS A TRAZAR
	dim3 gridR(conf.resolucion.x / conf.threads.x, conf.resolucion.y / conf.threads.y, 1);
	dim3 threadsR(conf.threads.x, conf.threads.y, 1);
	calcularRayos<<<gridR,threadsR>>>(d_listaRayos);

	//SE ACTUALIZA LA TEXTURA QUE CONTIENE LOS RAYOS
	textura_rayos.normalized = false;
	textura_rayos.filterMode = cudaFilterModePoint;
	textura_rayos.addressMode[0]= cudaAddressModeWrap;
	cudaChannelFormatDesc channelDesc = cudaCreateChannelDesc<float4>();
	CUDA_SAFE_CALL(cudaBindTexture(0, textura_rayos, d_listaRayos, channelDesc, conf.resolucion.x*conf.resolucion.y*sizeof(float4)));
}


extern "C" void initTexture(Escena* es, Configuracion conf){
	//SE COPIAN A CONSTANTES LOS VALORES
	CUDA_SAFE_CALL(cudaMemcpyToSymbol(configuracion_gpu,&conf,sizeof(Configuracion)));
	float cantLucesCopy = static_cast<float>(es->cant_luces);
	CUDA_SAFE_CALL(cudaMemcpyToSymbol(cant_luces,&(cantLucesCopy),sizeof(float)));
	CUDA_SAFE_CALL(cudaMemcpyToSymbol(bounding_box,&(es->grilla.bbEscena),sizeof(BoundingBox)));
	CUDA_SAFE_CALL(cudaMemcpyToSymbol(dimension_grilla,&(es->grilla.dimension),sizeof(float3)));
	float3 tam_v = (es->grilla.bbEscena.maximum-es->grilla.bbEscena.minimum)/es->grilla.dimension;
	CUDA_SAFE_CALL(cudaMemcpyToSymbol(tam_voxel,&tam_v,sizeof(float3)));
	float3 tam_g = (es->grilla.bbEscena.maximum-es->grilla.bbEscena.minimum)/2.0;
	CUDA_SAFE_CALL(cudaMemcpyToSymbol(tam_grilla,&tam_g,sizeof(float3)));

	//SE COPIAN LOS DATOS DE LOS TRIANGULOS A LA TEXTURA CORRESPONDIENTE
	float* d_triangulos;
	CUDA_SAFE_CALL( cudaMalloc((void**) &d_triangulos, 3 * es->cant_objetos *sizeof(float4)));
	CUDA_SAFE_CALL(cudaMemcpy(d_triangulos, es->triangulos, 3 * es->cant_objetos * sizeof(float4), cudaMemcpyHostToDevice));
	textura_triangulos.normalized = false;
	textura_triangulos.filterMode = cudaFilterModePoint;
	textura_triangulos.addressMode[0]= cudaAddressModeWrap;
	cudaChannelFormatDesc channelDesc = cudaCreateChannelDesc<float4>();
	size_t size = 3 * es->cant_objetos * sizeof(float4);
	CUDA_SAFE_CALL(cudaBindTexture(0, textura_triangulos, d_triangulos, channelDesc, size));

	//SE COPIAN LOS DATOS DE LAS NORMALES A LA TEXTURA CORRESPONDIENTE
	float* d_normales;
	CUDA_SAFE_CALL( cudaMalloc((void**) &d_normales, 3 * es->cant_objetos *sizeof(float4)));
	CUDA_SAFE_CALL(cudaMemcpy(d_normales, es->normales, 3 * es->cant_objetos * sizeof(float4), cudaMemcpyHostToDevice));
	textura_normales.normalized = false;
	textura_normales.filterMode = cudaFilterModePoint;
	textura_normales.addressMode[0]= cudaAddressModeWrap;
	CUDA_SAFE_CALL(cudaBindTexture(0, textura_normales, d_normales, channelDesc, size));

	//SE COPIAN LOS DATOS DE LOS VOXELS A LA TEXTURA CORRESPONDIENTE
	cudaChannelFormatDesc channelDescInt = cudaCreateChannelDesc<int>();
	int cant_voxels = es->grilla.dimension.x * es->grilla.dimension.y * es->grilla.dimension.z;
	int* d_voxels;
	CUDA_SAFE_CALL( cudaMalloc((void**) &d_voxels, cant_voxels *sizeof(int)));
	CUDA_SAFE_CALL(cudaMemcpy(d_voxels, es->grilla.voxels, cant_voxels * sizeof(int), cudaMemcpyHostToDevice));
	textura_voxels.normalized = false;
	textura_voxels.filterMode = cudaFilterModePoint;
	textura_voxels.addressMode[0]= cudaAddressModeWrap;
	CUDA_SAFE_CALL(cudaBindTexture(0, textura_voxels, d_voxels, channelDescInt, cant_voxels * sizeof(int)));
	
	//SE COPIAN LOS OBJETOS PERTENECIENTES A CADA VOXEL EN LA TEXTURA CORRESPONDIENTE
	int tam = -1;
	for(int i = 0; i<cant_voxels;i++){	
		tam = (tam>es->grilla.voxels[i])?tam:es->grilla.voxels[i];
	}
	while(es->grilla.listasGrid[tam]!=-1){
		tam++;
	}
	tam++;
	int* d_listasGrid;
	CUDA_SAFE_CALL( cudaMalloc((void**) &d_listasGrid, tam*sizeof(int)));
	CUDA_SAFE_CALL(cudaMemcpy(d_listasGrid, es->grilla.listasGrid, tam* sizeof(int), cudaMemcpyHostToDevice));
	textura_listasGrid.normalized = false;
	textura_listasGrid.filterMode = cudaFilterModePoint;
	textura_listasGrid.addressMode[0]= cudaAddressModeWrap;
	CUDA_SAFE_CALL(cudaBindTexture(0, textura_listasGrid, d_listasGrid, channelDescInt, tam * sizeof(int)));

	//SE COPIAN LOS MATERIALES A LA TEXTURA CORRESPONDIENTE
	float* d_materiales;
	CUDA_SAFE_CALL( cudaMalloc((void**) &d_materiales, es->cant_materiales*4*sizeof(float4)));
	CUDA_SAFE_CALL(cudaMemcpy(d_materiales, (es->materiales), es->cant_materiales*4*sizeof(float4), cudaMemcpyHostToDevice));
	textura_materiales.normalized = false;
	textura_materiales.filterMode = cudaFilterModePoint;
	textura_materiales.addressMode[0]= cudaAddressModeWrap;
	CUDA_SAFE_CALL(cudaBindTexture(0, textura_materiales, d_materiales, channelDesc, es->cant_materiales*4*sizeof(float4)));

	//SE INICIALIZAN LA LISTA DE RAYOS Y LA MATRIZ EN LA QUE SE DEVOLVERA EL RESULTADO DEL RAYTRACE
	CUDA_SAFE_CALL( cudaMalloc((void**) &d_color, sizeof(float3)* conf.resolucion.x * conf.resolucion.y));
	CUDA_SAFE_CALL( cudaMalloc((void**) &d_listaRayos, conf.resolucion.x * conf.resolucion.y * sizeof(float4)));
}

extern "C" void initLuces(Escena* es, Configuracion conf){
	cudaChannelFormatDesc channelDesc = cudaCreateChannelDesc<float4>();
	//SE INICIALIZA LA TEXTURA DE LAS LUCES
	float* d_luces;
	CUDA_SAFE_CALL( cudaMalloc((void**) &d_luces, 2 * es->cant_luces *sizeof(float4)));
	CUDA_SAFE_CALL(cudaMemcpy(d_luces, es->luces, 2 * es->cant_luces * sizeof(float4), cudaMemcpyHostToDevice));
	textura_luces.normalized = false;
	textura_luces.filterMode = cudaFilterModePoint;
	textura_luces.addressMode[0]= cudaAddressModeWrap;
	CUDA_SAFE_CALL(cudaBindTexture(0, textura_luces, d_luces, channelDesc, 2 * es->cant_luces * sizeof(float4)));
}	


extern "C" void raytrace(Escena *es, Configuracion conf, float3 *imagen){
	int ancho = conf.resolucion.x;
	int alto = conf.resolucion.y;
	int threads_x = conf.threads.x;
	int threads_y = conf.threads.y;

	dim3 grid(ancho/threads_x, alto/threads_y, 1);
	dim3 threads(threads_x, threads_y, 1);
	
	hallarColor<<<grid,threads>>>(d_color);
	cudaThreadSynchronize();
	
	//COPIA DEL RESULTADO A LA IMAGEN.
	CUDA_SAFE_CALL( cudaMemcpy(imagen, d_color, sizeof(float3) * ancho * alto , cudaMemcpyDeviceToHost));
}


#endif

