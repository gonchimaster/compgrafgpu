#include <cuda_runtime.h>
#include <cutil.h>
#include <vector_types.h>
#include <vector_functions.h>
#include <cutil_math.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "Escena.h"
#include "ListaEnteros.h"
#include "tipos.h"
#include "float.h"
#include "BMPSave.h"
#include "time.h"
#include "SDL/SDL.h"
#include "SDL/SDL_thread.h"
#include "CargadorDeConfiguracion.h"
#include "string.h";




#ifndef CUDA_ENABLED
extern "C" char** raytrace(Escena *escena);
#else
extern "C" void raytrace(Escena *escena, Configuracion conf, float3* imagen);
extern "C" void initTexture(Escena* es, Configuracion conf);
extern "C" void UpdateCamera(Escena* es, Configuracion conf);
extern "C" void initLuces(Escena* es, Configuracion conf);
#endif

Configuracion config;
double t = 0, x, y;
static bool bindedTextures;
static bool exited;

SDL_Surface *InitSDL(){
	SDL_Surface *screen;

	// Initialize SDL for video output 
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}

	//Inicializacion de la ventana de dibujo
	screen = SDL_SetVideoMode(config.resolucion.x, config.resolucion.y, 0, SDL_SWSURFACE | SDL_DOUBLEBUF);
	if (screen == NULL) {
		fprintf(stderr, "Unable to create screen: %s\n", SDL_GetError());
		SDL_Quit();
		exit(2);
	}
	
	return screen;
}

enum colores {R, G, B};

void PutPixel(SDL_Surface *superficie, int x, int y, Uint32 pixel) {

   // Obtenemos la profundidad de color

   int bpp = superficie->format->BytesPerPixel;


   // Obtenemos la posición del píxel a sustituir

   Uint8 *p = (Uint8 *)superficie->pixels + y * superficie->pitch + x*bpp;


   // Según sea la profundidad de color

   switch (bpp) {

    case 1: // 8 bits (256 colores)

	 *p = pixel;
	 break;
	
    case 2: // 16 bits (65536 colores o HighColor)
	 *(Uint16 *)p = pixel;
	 break;

    case 3: // 24 bits (True Color)

	 // Depende de la naturaleza del sistema
	 // Puede ser Big Endian o Little Endian
	 
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN) { 

	     // Calculamos cada una de las componentes de color
	     // 3 bytes, 3 posiciones

	     p[R]=(pixel >> 16) & 0xFF;
	     p[G]=(pixel >> 8) & 0xFF;
	     p[B]=pixel & 0xFF;
        }
	 else {

	     // Calculamos cada una de las componentes de color
	     // 3 byes, 3 posiciones

	     p[R]=pixel & 0xFF;
	     p[G]=(pixel >> 8) & 0xFF;
	     p[B]=(pixel >> 16) & 0xFF;

        }
	 break;

    case 4: // 32 bits (True Color + Alpha)

	  *(Uint32 *) p = pixel;
	  break;
    }
}

Escena escena2;
SDL_Surface *screen;
float3 *imagen;

bool increment_x, decrement_x;
bool increment_y, decrement_y;
bool increment_z, decrement_z;
bool aumenta = true, target = false;
float delta_value = 1.0f;

int CreateAndDisplayImage(void* pParams){
//TODO	SDL_Surface *image;
	clock_t t_inicial, t_final;
	double tiempoTotal = 0;
	int cantImagenes = 0;
	char* textoVentana;
	
	textoVentana = (char*)malloc(256 * sizeof(char));

	sprintf(textoVentana, "%d OBJ - [%d x %d x %d] GRID - [%d x %d] RES", escena2.cant_objetos, 
			(int)escena2.grilla.dimension.x, (int)escena2.grilla.dimension.y, (int)escena2.grilla.dimension.z,
			config.resolucion.x, config.resolucion.y);

	while(1) {
		//Titulo de la ventana SDL
		SDL_WM_SetCaption(textoVentana, NULL);

		t_inicial = clock();
		
		

#ifndef CUDA_ENABLED
		char **imagen = raytrace(&escena2);

		t_final = clock();

		Uint32 color = 0xFFFFFFFF;
		int i = 0;
		int j = 0;
		for(int y = 0;y < config.resolucion.y;y++){
			for(int x = 0;x < config.resolucion.x;x++){
				color = SDL_MapRGB(screen->format, imagen[i][j+2], imagen[i][j+1], imagen[i][j]);
				PutPixel(screen, x, y, color);
				j = j + 3;
			}
			i++;
			j = 0;
		}
#else
		if(!bindedTextures){
			printf("\nInicializando texturas...");
			initTexture(&escena2, config);
			initLuces(&escena2, config);
			bindedTextures = true;
			printf(" listo!!!\n");

			imagen = (float3*)malloc(config.resolucion.x * config.resolucion.y * sizeof(float3));
			
			printf("\nInicializando rayos...");
			UpdateCamera(&escena2, config);
			printf(" listo!!!\n");
		}

		raytrace(&escena2, config, imagen);
		
		t_final = clock();
		
		UpdateCamera(&escena2, config);

		if(decrement_x && target)
			escena2.camara.target.x -= delta_value;
		if(decrement_x && !target)
			escena2.camara.ojo.x -= delta_value;
		if(increment_x && target)
			escena2.camara.target.x += delta_value;
		if(increment_x && !target)
			escena2.camara.ojo.x += delta_value;
		if(decrement_y && target)
			escena2.camara.target.y -= delta_value;
		if(decrement_y && !target)
			escena2.camara.ojo.y -= delta_value;
		if(increment_y && target)
			escena2.camara.target.y += delta_value;
		if(increment_y && !target)
			escena2.camara.ojo.y += delta_value;
		if(decrement_z && target)
			escena2.camara.target.z -= delta_value;
		if(decrement_z && !target)
			escena2.camara.ojo.z -= delta_value;
		if(increment_z && target)
			escena2.camara.target.z += delta_value;
		if(increment_z && !target)
			escena2.camara.ojo.z += delta_value;

		Uint32 color = 0xFFFFFFFF;
		int i = 0;
		int j = 0;
		for(int y = 0;y < config.resolucion.y;y++){
			for(int x = 0;x < config.resolucion.x;x++){
				color = SDL_MapRGB(screen->format, 
					(Uint8)((imagen[y * config.resolucion.x + x].x > 1.0f ? 1 : imagen[y * config.resolucion.x + x].x) * 255), 
					(Uint8)((imagen[y * config.resolucion.x + x].y > 1.0f ? 1 : imagen[y * config.resolucion.x + x].y) * 255),
					(Uint8)((imagen[y * config.resolucion.x + x].z > 1.0f ? 1 : imagen[y * config.resolucion.x + x].z) * 255));
				PutPixel(screen, x, config.resolucion.y - 1 - y, color);
				j = j + 3;
			}
			i++;
			j = 0;
		}
#endif

		if(!exited ){
		SDL_Flip(screen);
		}
		if(!exited ){
		tiempoTotal = tiempoTotal + ((double)(t_final - t_inicial) / CLOCKS_PER_SEC);
		}
		if(!exited ){
		cantImagenes++;
		}
		if(!exited ){
			sprintf(textoVentana, "%d IMG - %.2f SEG - %.2f FPS - %d OBJ - [%d x %d x %d] GRID - [%d x %d] RES",
				cantImagenes, tiempoTotal, cantImagenes / tiempoTotal, escena2.cant_objetos, (int)escena2.grilla.dimension.x, (int)escena2.grilla.dimension.y,
			(int)escena2.grilla.dimension.z, config.resolucion.x, config.resolucion.y);
			if(target)
				textoVentana = strcat(&(textoVentana[0]), " - TARGET");
			else
				textoVentana = strcat(&(textoVentana[0]), " - EYE");
			if(aumenta)
				textoVentana = strcat(&(textoVentana[0]), " - INC");
			else
				textoVentana = strcat(&(textoVentana[0]), " - DEC");
		}
	}
	return 1;
}

int main(int argc, char **argv)
{
	clock_t t_ini, t_fin/*, t_tot*/;
	double secs;

	cargarConfiguracion("config.txt", config);

	t_ini = clock();

	printf("Iniciando la creación de la escena...\n");
	EscenaCrearDesdeArchivo(&escena2, config.nombreEscena);

	printf("\n\n");
	t_fin = clock();

	secs = (double)(t_fin - t_ini) / CLOCKS_PER_SEC;

	printf("Tiempo de armado: %f(s)\n", secs);

	screen = InitSDL();

	bindedTextures = false;
	exited = false;

	SDL_Thread *thread;
	thread = SDL_CreateThread(CreateAndDisplayImage, NULL);
	if (thread == NULL){
        fprintf(stderr, "No se puede crear el hilo: %s\n", SDL_GetError());
        exit(3);
    }

	bool salir = false;
	while(!salir){
		SDL_Event evento;
		SDLKey tecla;
		SDLMod modifier;
		while (SDL_PollEvent(&evento)) {
			tecla = evento.key.keysym.sym;
			switch(evento.type) {
				case SDL_KEYDOWN:
					switch(tecla){
						case SDLK_ESCAPE:
							SDL_KillThread(thread);
							salir = 1;
							exited = true;
							break;
						case SDLK_x:
							decrement_x = !aumenta;
							increment_x = !decrement_x;
							break;
						case SDLK_y:
							decrement_y = !aumenta;
							increment_y = !decrement_y;
							break;
						case SDLK_z:
							decrement_z = !aumenta;
							increment_z = !decrement_z;
							break;
						case SDLK_a:
							aumenta = !aumenta;
							break;
						case SDLK_t:
							target = !target;
							break;
						case SDLK_q:
							delta_value += 1.0f;
							break;
						case SDLK_w:
							delta_value -= 1.0f;
							break;
					}
					break;
				case SDL_KEYUP:
					switch(tecla){
						case SDLK_x:
							decrement_x = false;
							increment_x = false;
							break;
						case SDLK_y:
							decrement_y = false;
							increment_y = false;
							break;
						case SDLK_z:
							decrement_z = false;
							increment_z = false;
							break;
						case SDLK_DOWN:
							break;
					}
					break;
				case SDL_MOUSEMOTION:
					//printf ("Evento MOUSE MOVE. Coords (%d,%d)\n", evento.motion.x, evento.motion.y);
					break;
				case SDL_QUIT:
					SDL_KillThread(thread);
					salir = 1;
					exited = true;
					break;				
			}//end switch
		}//end while
	}
	
	SDL_Quit();
	return 1;
}

