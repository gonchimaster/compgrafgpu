#ifndef __ESCENA__H__
#define __ESCENA__H__

#include "UniformGrid.h"

typedef struct {
	float3 ojo;
	float3 target;
	float3 up;
} Camara;


typedef struct {
	float4 posicion;
	float4 color;
} Luz;

#ifndef CUDA_ENABLED

typedef struct {
	ObjetoEscena* objetos;
	int cant_objetos;
	Camara camara;
	Triangulo plano_de_vista;
	Luz* luces;
	int cant_luces;
	UniformGrid grilla;
	Material* materiales;
	int cant_materiales;
} Escena;

#else

typedef struct {
	//ObjetoEscena* objetos;//CAMBIA POR DOS ARREGLOS DE TRIANGULOS

	Triangulo* triangulos;//el w del vertice1 de cada uno de los triangulos 
						  //es utilizado para el id del material del objeto
	Triangulo* normales;
	//PARA COPIAR A LA MEM
	int cant_objetos;//NOT NEED IN GPU
	Camara camara; //CONST -> ojo, dx, dy, ini (float4)
	Triangulo plano_de_vista;//-î
	Luz* luces; //TEXTURA
	int cant_luces;//CONST
	UniformGrid grilla;//Copiar los arreglos como texturas y los valores como constantes
	Material* materiales;//Mirar la especificacion de Material, textura2D(w=4,h=cantMat)
	int cant_materiales;//NOT NEED IN GPU
} Escena;

#endif //CUDA_ENABLED


int EscenaCrearDesdeArchivo(Escena* escena, char* filename);
/* Crea la escena desde el archivo que la define. Retorna 0 en caso de error. */

void EscenaCrearDesdeHardcode(Escena* escena);
/* Crea la escena hardcodeada. */

#endif
