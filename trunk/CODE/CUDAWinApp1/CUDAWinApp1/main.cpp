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

int CreateAndDisplayImage(void* pParams){
//TODO	SDL_Surface *image;
	clock_t t_inicial, t_final;
	char textoVentana [100];
	sprintf(&(textoVentana[0]), "(-) FPS - %d OBJ - [%d x %d x %d] GRID - [%d x %d] RES", escena2.cant_objetos, 
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


		/////////////////
		//x = 10 * cos(t);
		//y = 10 * sin(t);
		//t = t + 0.1;

		//escena2.luces[0].posicion.x = x;
		//escena2.luces[0].posicion.y = y;
		//
		//printf("llega a modificar la camara\n");
		UpdateCamera(&escena2, config);
		/////////////////

		t_final = clock();

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

		SDL_Flip(screen);

		double secs = (double)(t_final - t_inicial) / CLOCKS_PER_SEC;
		
		sprintf(&(textoVentana[0]), "%f FPS - %d OBJ - [%d x %d x %d] GRID - [%d x %d] RES", 1 / secs, escena2.cant_objetos, 
			(int)escena2.grilla.dimension.x, (int)escena2.grilla.dimension.y, (int)escena2.grilla.dimension.z,
			config.resolucion.x, config.resolucion.y);
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
		while (SDL_PollEvent(&evento)) {
			switch(evento.type) {
				case SDL_KEYDOWN:
					tecla = evento.key.keysym.sym;
					//printf ("Evento KEY DOWN. Tecla=%d\n",tecla);
					switch(tecla){
						case SDLK_ESCAPE:
							SDL_KillThread(thread);
							salir = 1;
							break;
						case SDLK_LEFT:
							escena2.camara.ojo.y -= 1.0f;
							break;
						case SDLK_RIGHT:
							escena2.camara.ojo.y += 1.0f;
							break;
						case SDLK_UP:
							escena2.camara.ojo.x -= 5.0f;
							escena2.plano_de_vista.v1.x -= 5.0f;
							escena2.plano_de_vista.v2.x -= 5.0f;
							escena2.plano_de_vista.v3.x -= 5.0f;
							break;
						case SDLK_DOWN:
							escena2.camara.ojo.x += 5.0f;
							escena2.plano_de_vista.v1.x += 5.0f;
							escena2.plano_de_vista.v2.x += 5.0f;
							escena2.plano_de_vista.v3.x += 5.0f;
							break;
						}
					break;
				case SDL_KEYUP:
					//printf ("Evento KEY UP. Tecla=%d\n",evento.key.keysym.sym);					
					break;
				case SDL_MOUSEMOTION:
					//printf ("Evento MOUSE MOVE. Coords (%d,%d)\n", evento.motion.x, evento.motion.y);
					break;
				case SDL_QUIT:
					SDL_KillThread(thread);
					salir = 1;
					break;				
			}//end switch
		}//end while
	}
	
	SDL_Quit();
	return 1;
}

