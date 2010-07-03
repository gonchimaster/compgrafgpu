#include "CargadorDeConfiguracion.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cargarConfiguracion(char* fileName, Configuracion& conf){
	FILE* archivoConfig = fopen(fileName,"r");
	int cont=0;

	char* linea = (char*) malloc(2048*sizeof(char));
	char* command = (char*) malloc(32*sizeof(char));
	char* param = (char*) malloc(2016*sizeof(char));;

	
	while(!feof(archivoConfig)){

		fgets(linea, 2047, archivoConfig);
		sscanf(linea,"%[^=]=%[^\n]\n", command, param);

		if(strcmp(command, "#")==0){
			//printf("COMMENT\n");
		}
		else if(strcmp(command, "TAMANIO_GRILLA.X")==0){
			conf.tamanio_grilla.x = atoi(param);
		}
		else if(strcmp(command, "TAMANIO_GRILLA.Y")==0){
			conf.tamanio_grilla.y = atoi(param);
		}
		else if(strcmp(command, "TAMANIO_GRILLA.Z")==0){
			conf.tamanio_grilla.z = atoi(param);
		}
		else if(strcmp(command, "RESOLUCION.X")==0){
			conf.resolucion.x = atoi(param);
		}
		else if(strcmp(command, "RESOLUCION.Y")==0){
			conf.resolucion.y = atoi(param);
		}
		else if(strcmp(command, "INFINITO")==0){
			conf.INFINITO = (float)(atof(param));
		}
		else if(strcmp(command, "ZERO")==0){
			conf.ZERO = (float)(atof(param));
		}
		else if(strcmp(command, "PROFUNDIDAD_RECURSION")==0){
			conf.profundidad_recursion = atoi(param);
		}
		else if(strcmp(command, "THREADS.X")==0){
			conf.threads.x = atoi(param);
		}
		else if(strcmp(command, "THREADS.Y")==0){
			conf.threads.y = atoi(param);
		}
		else if(strcmp(command, "THREADS.Z")==0){
			conf.threads.z = atoi(param);
		}
		else if(strcmp(command, "ESCENA")==0){
			strcpy(&(conf.nombreEscena[0]),param);
		}
		else{
			//printf("C = %s P = %s\n", command, param);
		}
		
		
	}

	free(linea);

	fclose(archivoConfig);
	
}
