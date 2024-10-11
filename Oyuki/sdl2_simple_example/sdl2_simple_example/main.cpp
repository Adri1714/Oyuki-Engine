#include <GL/glew.h>
#include <chrono>
#include <thread>
#include <exception>
#include <glm/glm.hpp>
#include <SDL2/SDL_events.h>
#include "MyWindow.h"
#include <stdio.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>


using namespace std;

using hrclock = chrono::high_resolution_clock;
using u8vec4 = glm::u8vec4;
using ivec2 = glm::ivec2;
using vec3 = glm::dvec3;

static const ivec2 WINDOW_SIZE(512, 512);
static const unsigned int FPS = 60;
static const auto FRAME_DT = 1.0s / FPS;

static void init_openGL() {
	glewInit();
	if (!GLEW_VERSION_3_0) throw exception("OpenGL 3.0 API is not available.");
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.5, 0.5, 0.5, 1.0);
}

static void draw_triangle(const u8vec4& color, const vec3& center, double size) {
	glColor4ub(color.r, color.g, color.b, color.a);
	glBegin(GL_TRIANGLES);
	glVertex3d(center.x, center.y + size, center.z);
	glVertex3d(center.x - size, center.y - size, center.z);
	glVertex3d(center.x + size, center.y - size, center.z);
	glEnd();
}

static void draw_cube( const vec3& center, double size) {
	
	glBegin(GL_TRIANGLES);  // draw a cube with 12 triangles
	float v0[3] = { center.x + 0.3f,center.y + 0.3f,center.z- 0.3f };
	float v1[3] = { center.x- 0.3f,center.y + 0.3f,center.z- 0.3f };
	float v2[3] = { center.x - 0.3f,center.y- 0.3f,center.z- 0.3f };
	float v3[3] = { center.x + 0.3f,center.y - 0.3f,center.z - 0.3f };
	float v4[3] = { center.x + 0.3f,center.y - 0.3f,center.z+ 0.3f };
	float v5[3] = { center.x + 0.3f,center.y + 0.3f,center.z+ 0.3f };
	float v6[3] = { center.x - 0.3f,center.y + 0.3f,center.z+ 0.3f };
	float v7[3] = { center.x - 0.3f,center.y - 0.3f,center.z+ 0.3f };
	// front face =================
	glColor3f(0, 0, 1);
	glVertex3fv(v0);    // v0-v1-v2
	glColor3f(0, 0, 1);
	glVertex3fv(v1);
	glColor3f(0, 0, 1);
	glVertex3fv(v2);

	glColor3f(0, 0, 1);
	glVertex3fv(v2);    // v2-v3-v0
	glColor3f(0, 0, 1);
	glVertex3fv(v3);
	glColor3f(0, 0, 1);
	glVertex3fv(v0);

	// right face =================
	glColor3f(1, 0, 0);
	glVertex3fv(v0);    // v0-v3-v4
	glVertex3fv(v3);
	glVertex3fv(v4);

	glVertex3fv(v4);    // v4-v5-v0
	glVertex3fv(v5);
	glVertex3fv(v0);

	// top face ===================
	glColor3f(0, 1, 0);
	glVertex3fv(v0);    // v0-v5-v6
	glVertex3fv(v5);
	glVertex3fv(v6);

	glVertex3fv(v6);    // v6-v1-v0
	glVertex3fv(v1);
	glVertex3fv(v0);

	// left face ===================
	glColor3f(0, 0, 0);
	glVertex3fv(v1);    
	glVertex3fv(v6);
	glVertex3fv(v7);

	glVertex3fv(v1);    // v6-v1-v0
	glVertex3fv(v7);
	glVertex3fv(v2);

	// bottom face ===================
	glColor3f(1, 1, 1);
	glVertex3fv(v3);   
	glVertex3fv(v4);
	glVertex3fv(v7);
	
	glVertex3fv(v7);    
	glVertex3fv(v2);
	glVertex3fv(v3);

	// back face ===================
	glColor3f(1, 0, 1);
	glVertex3fv(v5);
	glVertex3fv(v7);
	glVertex3fv(v6);

	glVertex3fv(v5);
	glVertex3fv(v4);
	glVertex3fv(v7);

		glEnd();
}
struct MeshData {
	vector<vec3> vertices;
	vector<vector<unsigned int>> triangles;
};
static void drawModel(vector<MeshData> data) {
	// Verificar que el número de vértices es un múltiplo de 3
	//if (vertices.size() % 3 != 0) {
		//fprintf(stderr, "Error: El número de vértices no es un múltiplo de 3.\n");
		//return; // Salir de la función si no se cumplen las condiciones
	//}

	glBegin(GL_TRIANGLES); // Asumimos que los vértices están organizados en triángulos
	for (size_t a = 0; a < data.size(); a++)
	{
		for (size_t i = 0; i < data[a].triangles.size(); i++) {
			// Establecer color para cada triángulo
			if (i / 3 % 6 == 0) glColor3f(1.0f, 0.0f, 0.0f); // Rojo
			else if (i / 3 % 6 == 1) glColor3f(0.0f, 1.0f, 0.0f); // Verde
			else if (i / 3 % 6 == 2) glColor3f(0.0f, 0.0f, 1.0f); // Azul
			else if (i / 3 % 6 == 3) glColor3f(1.0f, 1.0f, 0.0f); // Amarillo
			else if (i / 3 % 6 == 4) glColor3f(1.0f, 0.0f, 1.0f); // Magenta
			else if (i / 3 % 6 == 5) glColor3f(0.0f, 1.0f, 1.0f); // Cian

			glVertex3f((data[a].vertices[data[a].triangles[i][0]].x), (data[a].vertices[data[a].triangles[i][0]].y), (data[a].vertices[data[a].triangles[i][0]].z));
			glVertex3f((data[a].vertices[data[a].triangles[i][1]].x), (data[a].vertices[data[a].triangles[i][1]].y), (data[a].vertices[data[a].triangles[i][1]].z));
			glVertex3f((data[a].vertices[data[a].triangles[i][2]].x), (data[a].vertices[data[a].triangles[i][2]].y), (data[a].vertices[data[a].triangles[i][2]].z));

		}
	}
	

	glEnd();
}

vector<MeshData> CubitoFbx()
{
	const char* file = "C:/Users/adriarj/Downloads/putin.fbx"; // Ruta del fitxer a carregar
	const struct aiScene* scene = aiImportFile(file, aiProcess_Triangulate);
	const float scaleFactor = 0.5f;
	if (!scene) {
		fprintf(stderr, "Error en carregar el fitxer: %s\n", aiGetErrorString());
		return {};
	}
	vector<vec3> vertices;                  // Vértices
	vector<vector<unsigned int>> triangles; // Índices de triángulos
	printf("Numero de malles: %u\n", scene->mNumMeshes);

	
	vector<MeshData> MayaTotal;
	unsigned int vertexOffset = 0;

	for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[i];
		MeshData meshData;
		printf("\nMalla %u:\n", i);
		printf(" Numero de vertexs: %u\n", mesh->mNumVertices);
		printf(" Numero de triangles: %u\n", mesh->mNumFaces);
		// Vèrtexs
		for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
			aiVector3D vertex = mesh->mVertices[v];
			printf(" Vertex %u: (%f, %f, %f)\n", v, vertex.x* scaleFactor, vertex.y*scaleFactor, vertex.z* scaleFactor);
			meshData.vertices.push_back({ vertex.x * scaleFactor, vertex.y * scaleFactor, vertex.z * scaleFactor });
		}
		// Índexs de triangles (3 per triangle)
		for (unsigned int f = 0; f < mesh->mNumFaces; f++) {

			aiFace face = mesh->mFaces[f];
			vector<unsigned int> indices;
			printf(" Indexs triangle %u: ", f);

			for (unsigned int j = 0; j < face.mNumIndices; j++) {
				indices.push_back(face.mIndices[j] );
				printf("%u ", face.mIndices[j]);
			}
			meshData.triangles.push_back(indices);
			printf("\n");
		}
		MayaTotal.push_back(meshData);
	}
	aiReleaseImport(scene);
	return MayaTotal;

}
vector<MeshData> dato;
static void display_func() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//draw_triangle(u8vec4(255, 0, 0, 255), vec3(0.0, 0.0, 0.0), 0.5);
	//draw_cube( vec3(0.0, 0.0, 0.0), 0.5);
	drawModel(dato);
	glRotatef(0.5f, 0.0f, 1.0f, 0.0f);

}

#include "imgui_impl_sdl2.h"

static bool processEvents() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			return false;
			break;
		default:
			ImGui_ImplSDL2_ProcessEvent(&event);
			break;
		}

	}
	return true;
}

int main(int argc, char** argv) {
	MyWindow window("SDL2 Simple Example", WINDOW_SIZE.x, WINDOW_SIZE.y);
	
	dato = CubitoFbx(); // Cargar los vértices solo una vez

	init_openGL();
	while (processEvents()) {
		const auto t0 = hrclock::now();
		display_func();
		window.draw();
		//window.swapBuffers();
		const auto t1 = hrclock::now();
		const auto dt = t1 - t0;
		if(dt<FRAME_DT) this_thread::sleep_for(FRAME_DT - dt);
	}

	return 0;
}