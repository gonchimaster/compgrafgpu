#include <stdlib.h>
#include "obj_parser.h"
#include "Escena.h"
#include "UniformGrid.h"
#include <stdio.h>
#include <cuda_runtime.h>
#include <cutil.h>
#include <vector_types.h>
#include <vector_functions.h>
#include <cutil_math.h>

#define MAX_OBJETOS 10

#ifndef CUDA_ENABLED

void EscenaCrearUniformGrid(ObjetoEscena* escena, int topeEscena, UniformGrid* grilla){
	UniformGridCrear(escena, topeEscena, grilla);
}

/* Crea la escena hardcodeada. */
void EscenaCrearDesdeHardcode(Escena* escena){
	
	//Objetos
	escena->objetos = (ObjetoEscena*) malloc(sizeof(ObjetoEscena) * MAX_OBJETOS);
	escena->cant_objetos = 0;

	escena->objetos[escena->cant_objetos].tipo = Triangle;
	float4 v1, v2, v3;
	v1 = make_float4(10, 10, 10, 0);
	v2 = make_float4(11, 11, 11, 0);
	v3  = make_float4(5, 5, 5, 0);
	TrianguloSetVertices(&(escena->objetos[escena->cant_objetos].tri), v1, v2, v3);
	escena->cant_objetos++;
	
	escena->objetos[escena->cant_objetos].tipo = Triangle;
	v1 = make_float4(8, 8, 8, 0);
	v2 = make_float4(2, 2, 2, 0);
	v3 = make_float4(7, 7, 7, 0);
	TrianguloSetVertices(&(escena->objetos[escena->cant_objetos].tri), v1, v2, v3);
	escena->cant_objetos++;

	escena->objetos[escena->cant_objetos].tipo = Triangle;
	v1 = make_float4(15, 15, 15, 0);
	v2 = make_float4(8, 11, 9, 0);
	v3 = make_float4(5, 5, 5, 0);
	TrianguloSetVertices(&(escena->objetos[escena->cant_objetos].tri), v1, v2, v3);
	escena->cant_objetos++;

	escena->objetos[escena->cant_objetos].tipo = Triangle;
	v1 = make_float4(2.1, 2.2, 2.3, 0);
	v2 = make_float4(3.1, 3.1, 3.1, 0);
	v3 = make_float4(2, 3, 2, 0);
	TrianguloSetVertices(&(escena->objetos[escena->cant_objetos].tri), v1, v2, v3);
	escena->cant_objetos++;

	//Luces se cargan en escena->luces, por ahora no hay ninguna
	escena->cant_luces = 0;

	//Aca hay que setear la camara

	//Aca hay que setear el plano de vista

	//Grilla
	EscenaCrearUniformGrid(escena->objetos, escena->cant_objetos, &(escena->grilla));
}

/* Crea la escena desde el archivo que la define. Retorna 0 en caso de error. */
int EscenaCrearDesdeArchivo(Escena* escena, char* filename){
	obj_scene_data objData;
	int result = parse_obj_scene(&objData, filename);
	if(!result)
		return 0;

	//Objetos
	escena->objetos = (ObjetoEscena*) malloc(sizeof(ObjetoEscena) * (objData.face_count + objData.sphere_count));
	escena->cant_objetos = 0;
	
	//Cargo los triangulos...
	for(int indiceFace = 0;indiceFace < objData.face_count;indiceFace++){
		escena->objetos[escena->cant_objetos].tipo = Triangle;
		
		float4 v1, v2, v3;
		v1 = make_float4(
			(float)objData.vertex_list[objData.face_list[indiceFace]->vertex_index[0]]->e[0],
			(float)objData.vertex_list[objData.face_list[indiceFace]->vertex_index[0]]->e[1],
			(float)objData.vertex_list[objData.face_list[indiceFace]->vertex_index[0]]->e[2],
			0);
		v2 = make_float4(
			(float)objData.vertex_list[objData.face_list[indiceFace]->vertex_index[1]]->e[0],
			(float)objData.vertex_list[objData.face_list[indiceFace]->vertex_index[1]]->e[1],
			(float)objData.vertex_list[objData.face_list[indiceFace]->vertex_index[1]]->e[2],
			0);
		v3 = make_float4(
			(float)objData.vertex_list[objData.face_list[indiceFace]->vertex_index[2]]->e[0],
			(float)objData.vertex_list[objData.face_list[indiceFace]->vertex_index[2]]->e[1],
			(float)objData.vertex_list[objData.face_list[indiceFace]->vertex_index[2]]->e[2],
			0);
		
		TrianguloSetVertices(&(escena->objetos[escena->cant_objetos].tri), v1, v2, v3);

		v1 = make_float4(
			(float)objData.vertex_normal_list[objData.face_list[indiceFace]->normal_index[0]]->e[0],
			(float)objData.vertex_normal_list[objData.face_list[indiceFace]->normal_index[0]]->e[1],
			(float)objData.vertex_normal_list[objData.face_list[indiceFace]->normal_index[0]]->e[2],
			0);
		v2 = make_float4(
			(float)objData.vertex_normal_list[objData.face_list[indiceFace]->normal_index[1]]->e[0],
			(float)objData.vertex_normal_list[objData.face_list[indiceFace]->normal_index[1]]->e[1],
			(float)objData.vertex_normal_list[objData.face_list[indiceFace]->normal_index[1]]->e[2],
			0);
		v3 = make_float4(
			(float)objData.vertex_normal_list[objData.face_list[indiceFace]->normal_index[2]]->e[0],
			(float)objData.vertex_normal_list[objData.face_list[indiceFace]->normal_index[2]]->e[1],
			(float)objData.vertex_normal_list[objData.face_list[indiceFace]->normal_index[2]]->e[2],
			0);
		
		TrianguloSetVertices(&(escena->objetos[escena->cant_objetos].normales), v1, v2, v3);
		
		escena->objetos[escena->cant_objetos].id_material = objData.face_list[indiceFace]->material_index;

		escena->cant_objetos++;
	}

	//Cargo las esferas...
	for(int indiceEsf = 0;indiceEsf < objData.sphere_count;indiceEsf++){
		escena->objetos[escena->cant_objetos].tipo = Sphere;
		
		//En el primer vertice del triangulo esta el centro de la esfera...
		escena->objetos[escena->cant_objetos].tri.v1 = make_float4(
			(float)objData.vertex_list[objData.sphere_list[indiceEsf]->pos_index]->e[0],
			(float)objData.vertex_list[objData.sphere_list[indiceEsf]->pos_index]->e[1],
			(float)objData.vertex_list[objData.sphere_list[indiceEsf]->pos_index]->e[2],
			0);
		
		//La componente X de del segundo vertice del triangulo es el radio de la esfera...
		float3 upNormal;
		upNormal = make_float3(
			(float)objData.vertex_list[objData.sphere_list[indiceEsf]->up_normal_index]->e[0],
			(float)objData.vertex_list[objData.sphere_list[indiceEsf]->up_normal_index]->e[1],
			(float)objData.vertex_list[objData.sphere_list[indiceEsf]->up_normal_index]->e[2]);
		
		escena->objetos[escena->cant_objetos].tri.v2.x = length(upNormal);

		escena->objetos[escena->cant_objetos].id_material = objData.face_list[indiceEsf]->material_index;
		escena->cant_objetos++;
	}
	
	//Aca hay que setear la camara
	escena->camara.ojo = make_float3(
			objData.vertex_list[objData.camera->camera_pos_index]->e[0],
			objData.vertex_list[objData.camera->camera_pos_index]->e[1],
			objData.vertex_list[objData.camera->camera_pos_index]->e[2]);
	
	float3 target;
	target = make_float3(
			objData.vertex_list[objData.camera->camera_look_point_index]->e[0],
			objData.vertex_list[objData.camera->camera_look_point_index]->e[1],
			objData.vertex_list[objData.camera->camera_look_point_index]->e[2]);
	
	escena->camara.direccion =	target - escena->camara.ojo;
	
	escena->camara.up = make_float3(
			objData.vertex_normal_list[objData.camera->camera_up_norm_index]->e[0],
			objData.vertex_normal_list[objData.camera->camera_up_norm_index]->e[1],
			objData.vertex_normal_list[objData.camera->camera_up_norm_index]->e[2]);

	//TODO: Aca hay que setear el plano de vista
	escena->plano_de_vista.v1 = make_float4(
			objData.vertex_list[objData.light_quad_list[0]->vertex_index[0]]->e[0],
			objData.vertex_list[objData.light_quad_list[0]->vertex_index[0]]->e[1],
			objData.vertex_list[objData.light_quad_list[0]->vertex_index[0]]->e[2],
			0);
	escena->plano_de_vista.v2 = make_float4(
			objData.vertex_list[objData.light_quad_list[0]->vertex_index[1]]->e[0],
			objData.vertex_list[objData.light_quad_list[0]->vertex_index[1]]->e[1],
			objData.vertex_list[objData.light_quad_list[0]->vertex_index[1]]->e[2],
			0);
	escena->plano_de_vista.v3 = make_float4(
			objData.vertex_list[objData.light_quad_list[0]->vertex_index[2]]->e[0],
			objData.vertex_list[objData.light_quad_list[0]->vertex_index[2]]->e[1],
			objData.vertex_list[objData.light_quad_list[0]->vertex_index[2]]->e[2],
			0);
	
	//Luces se cargan en escena->luces
	escena->luces = (Luz*) malloc(sizeof(Luz) * objData.light_point_count);
	escena->cant_luces = 0;
	for(int indexLuz = 0;indexLuz < objData.light_point_count;indexLuz++){
		escena->luces[indexLuz].posicion = make_float4(
			objData.vertex_list[objData.light_point_list[indexLuz]->pos_index]->e[0],
			objData.vertex_list[objData.light_point_list[indexLuz]->pos_index]->e[1],
			objData.vertex_list[objData.light_point_list[indexLuz]->pos_index]->e[2],
			0);
		
		escena->luces[indexLuz].color = make_float4(
			objData.material_list[objData.light_point_list[indexLuz]->material_index]->diff[0],
			objData.material_list[objData.light_point_list[indexLuz]->material_index]->diff[1],
			objData.material_list[objData.light_point_list[indexLuz]->material_index]->diff[2],
			0);
		
		escena->cant_luces++;
	}	

	//Grilla
	EscenaCrearUniformGrid(escena->objetos, escena->cant_objetos, &(escena->grilla));

	//Materiales
	escena->materiales = (Material*) malloc(sizeof(Material) * objData.material_count);
	escena->cant_materiales = 0;
	for(int indiceMat = 0;indiceMat < objData.material_count;indiceMat++){
		escena->materiales[indiceMat].diffuse_color = make_float3(
			objData.material_list[indiceMat]->diff[0],
			objData.material_list[indiceMat]->diff[1],
			objData.material_list[indiceMat]->diff[2]);

		escena->materiales[indiceMat].ambient_color = make_float3(
			objData.material_list[indiceMat]->amb[0],
			objData.material_list[indiceMat]->amb[1],
			objData.material_list[indiceMat]->amb[2]);

		escena->materiales[indiceMat].specular_color = make_float3(
			objData.material_list[indiceMat]->spec[0],
			objData.material_list[indiceMat]->spec[1],
			objData.material_list[indiceMat]->spec[2]);

		escena->materiales[indiceMat].refraction = (float)objData.material_list[indiceMat]->refract;
		escena->materiales[indiceMat].reflection = (float)objData.material_list[indiceMat]->reflect;
		escena->materiales[indiceMat].transparency = 1.0-(float)objData.material_list[indiceMat]->trans;
		escena->materiales[indiceMat].coef_at_especular = (float)objData.material_list[indiceMat]->shiny;
		escena->materiales[indiceMat].index_of_refraction = (float)objData.material_list[indiceMat]->refract_index;
		
		escena->cant_materiales++;
	}

	delete_obj_data(&objData);
	return 1;
}

#else

void EscenaCrearUniformGrid(Triangulo* triangulos, int topeEscena, UniformGrid* grilla){
	UniformGridCrear(triangulos, topeEscena, grilla);
}

/* Crea la escena desde el archivo que la define. Retorna 0 en caso de error. */
int EscenaCrearDesdeArchivo(Escena* escena, char* filename){
	obj_scene_data objData;
	int result = parse_obj_scene(&objData, filename);
	if(!result)
		return 0;

	//Objetos
	escena->triangulos = (Triangulo*) malloc(sizeof(Triangulo) * objData.face_count);
	escena->normales = (Triangulo*) malloc(sizeof(Triangulo) * objData.face_count);
	escena->cant_objetos = 0;

	//Cargo los triangulos...
	for(int indiceFace = 0;indiceFace < objData.face_count;indiceFace++){
		float4 v1, v2, v3;
		v1 = make_float4(
			(float)objData.vertex_list[objData.face_list[indiceFace]->vertex_index[0]]->e[0],
			(float)objData.vertex_list[objData.face_list[indiceFace]->vertex_index[0]]->e[1],
			(float)objData.vertex_list[objData.face_list[indiceFace]->vertex_index[0]]->e[2],
			(float)objData.face_list[indiceFace]->material_index);
		v2 = make_float4(
			(float)objData.vertex_list[objData.face_list[indiceFace]->vertex_index[1]]->e[0],
			(float)objData.vertex_list[objData.face_list[indiceFace]->vertex_index[1]]->e[1],
			(float)objData.vertex_list[objData.face_list[indiceFace]->vertex_index[1]]->e[2],
			0);
		v3 = make_float4(
			(float)objData.vertex_list[objData.face_list[indiceFace]->vertex_index[2]]->e[0],
			(float)objData.vertex_list[objData.face_list[indiceFace]->vertex_index[2]]->e[1],
			(float)objData.vertex_list[objData.face_list[indiceFace]->vertex_index[2]]->e[2],
			0);
		
		TrianguloSetVertices(&(escena->triangulos[escena->cant_objetos]), v1, v2, v3);

		v1 = make_float4(
			(float)objData.vertex_normal_list[objData.face_list[indiceFace]->normal_index[0]]->e[0],
			(float)objData.vertex_normal_list[objData.face_list[indiceFace]->normal_index[0]]->e[1],
			(float)objData.vertex_normal_list[objData.face_list[indiceFace]->normal_index[0]]->e[2],
			0);
		v2 = make_float4(
			(float)objData.vertex_normal_list[objData.face_list[indiceFace]->normal_index[1]]->e[0],
			(float)objData.vertex_normal_list[objData.face_list[indiceFace]->normal_index[1]]->e[1],
			(float)objData.vertex_normal_list[objData.face_list[indiceFace]->normal_index[1]]->e[2],
			0);
		v3 = make_float4(
			(float)objData.vertex_normal_list[objData.face_list[indiceFace]->normal_index[2]]->e[0],
			(float)objData.vertex_normal_list[objData.face_list[indiceFace]->normal_index[2]]->e[1],
			(float)objData.vertex_normal_list[objData.face_list[indiceFace]->normal_index[2]]->e[2],
			0);
		
		TrianguloSetVertices(&(escena->normales[escena->cant_objetos]), v1, v2, v3);

		escena->cant_objetos++;
	}
	
	//Aca hay que setear la camara
	escena->camara.ojo = make_float3(
			(float)objData.vertex_list[objData.camera->camera_pos_index]->e[0],
			(float)objData.vertex_list[objData.camera->camera_pos_index]->e[1],
			(float)objData.vertex_list[objData.camera->camera_pos_index]->e[2]);
	
	float3 target;
	target = make_float3(
			(float)objData.vertex_list[objData.camera->camera_look_point_index]->e[0],
			(float)objData.vertex_list[objData.camera->camera_look_point_index]->e[1],
			(float)objData.vertex_list[objData.camera->camera_look_point_index]->e[2]);
	
	escena->camara.target =	target;
	
	escena->camara.up = make_float3(
			(float)objData.vertex_normal_list[objData.camera->camera_up_norm_index]->e[0],
			(float)objData.vertex_normal_list[objData.camera->camera_up_norm_index]->e[1],
			(float)objData.vertex_normal_list[objData.camera->camera_up_norm_index]->e[2]);

	//TODO: Aca hay que setear el plano de vista
	escena->plano_de_vista.v1 = make_float4(
			(float)objData.vertex_list[objData.light_quad_list[0]->vertex_index[0]]->e[0],
			(float)objData.vertex_list[objData.light_quad_list[0]->vertex_index[0]]->e[1],
			(float)objData.vertex_list[objData.light_quad_list[0]->vertex_index[0]]->e[2],
			0.f);
	escena->plano_de_vista.v2 = make_float4(
			(float)objData.vertex_list[objData.light_quad_list[0]->vertex_index[1]]->e[0],
			(float)objData.vertex_list[objData.light_quad_list[0]->vertex_index[1]]->e[1],
			(float)objData.vertex_list[objData.light_quad_list[0]->vertex_index[1]]->e[2],
			0.f);
	escena->plano_de_vista.v3 = make_float4(
			(float)objData.vertex_list[objData.light_quad_list[0]->vertex_index[2]]->e[0],
			(float)objData.vertex_list[objData.light_quad_list[0]->vertex_index[2]]->e[1],
			(float)objData.vertex_list[objData.light_quad_list[0]->vertex_index[2]]->e[2],
			0.f);
	
	//Luces se cargan en escena->luces
	escena->luces = (Luz*) malloc(sizeof(Luz) * objData.light_point_count);
	escena->cant_luces = 0;
	for(int indexLuz = 0;indexLuz < objData.light_point_count;indexLuz++){
		escena->luces[indexLuz].posicion = make_float4(
			(float)objData.vertex_list[objData.light_point_list[indexLuz]->pos_index]->e[0],
			(float)objData.vertex_list[objData.light_point_list[indexLuz]->pos_index]->e[1],
			(float)objData.vertex_list[objData.light_point_list[indexLuz]->pos_index]->e[2],0.f);
		
		escena->luces[indexLuz].color = make_float4(
			(float)objData.material_list[objData.light_point_list[indexLuz]->material_index]->diff[0],
			(float)objData.material_list[objData.light_point_list[indexLuz]->material_index]->diff[1],
			(float)objData.material_list[objData.light_point_list[indexLuz]->material_index]->diff[2],0.f);
		
		escena->cant_luces++;
	}	

	//Grilla
	EscenaCrearUniformGrid(escena->triangulos, escena->cant_objetos, &(escena->grilla));

	//Materiales
	escena->materiales = (Material*) malloc(sizeof(Material) * objData.material_count);
	escena->cant_materiales = 0;
	for(int indiceMat = 0;indiceMat < objData.material_count;indiceMat++){
		escena->materiales[indiceMat].diffuse_color = make_float4(
			(float)objData.material_list[indiceMat]->diff[0],
			(float)objData.material_list[indiceMat]->diff[1],
			(float)objData.material_list[indiceMat]->diff[2],
			(float)objData.material_list[indiceMat]->shiny);

		escena->materiales[indiceMat].ambient_color = make_float4(
			(float)objData.material_list[indiceMat]->amb[0],
			(float)objData.material_list[indiceMat]->amb[1],
			(float)objData.material_list[indiceMat]->amb[2],
			0.f);

		escena->materiales[indiceMat].specular_color = make_float4(
			(float)objData.material_list[indiceMat]->spec[0],
			(float)objData.material_list[indiceMat]->spec[1],
			(float)objData.material_list[indiceMat]->spec[2],
			0.f);

		//(refraction, reflection, transparency, index_of_refraction)
		escena->materiales[indiceMat].other = make_float4((float)objData.material_list[indiceMat]->refract,
														  (float)objData.material_list[indiceMat]->reflect,
														  1.0f - (float)objData.material_list[indiceMat]->trans,
														  (float)objData.material_list[indiceMat]->refract_index);
		escena->cant_materiales++;
	}

	delete_obj_data(&objData);
	return 1;
}

#endif