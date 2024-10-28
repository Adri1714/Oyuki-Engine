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
#include "imgui_impl_sdl2.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <IL/il.h>

using namespace std;

using hrclock = chrono::high_resolution_clock;
using u8vec4 = glm::u8vec4;
using ivec2 = glm::ivec2;
using vec3 = glm::dvec3;

static const ivec2 WINDOW_SIZE(512, 512);
static const unsigned int FPS = 60;
static const auto FRAME_DT = 1.0s / FPS;

glm::mat4 projectionMatrix;
glm::mat4 viewMatrix;
glm::mat4 modelMatrix;



static void init_openGL() {
	glewInit();
	if (!GLEW_VERSION_3_0) throw exception("OpenGL 3.0 API is not available.");

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	ilInit();
	/*glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_NORMALIZE);  // Normaliza las normales automáticamente

	// Configuración de la luz
	GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 0.0f };  // Luz direccional
	GLfloat light_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	GLfloat light_diffuse[] = { 0.7f, 0.7f, 0.7f, 1.0f };
	GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);*/

	/*// Configuración del material
	GLfloat mat_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	GLfloat mat_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat mat_shininess[] = { 50.0f };

	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);*/

	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);  // Fondo más oscuro para mejor contraste

	projectionMatrix = glm::perspective(glm::radians(45.0f),
		static_cast<float>(WINDOW_SIZE.x) / WINDOW_SIZE.y, 0.1f, 100.0f);
	viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.0f));
	modelMatrix = glm::mat4(1.0f);
}

struct MeshData // Estructura para almacenar los datos de un modelo 3D
{
	vector<vec3> vertices;
	vector<vector<unsigned int>> triangles;
	vector<vec3> colors;
	vector<vec3> normals; 
	vector<vec3> texCoords;
	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint ebo = 0;
	GLuint normalVBO = 0;
	GLuint textureVBO = 0;
	GLuint colorVBO = 0;
};

static void drawModel(const vector<MeshData>& data) {
	
	for (const auto& meshData : data) {
		glBindVertexArray(meshData.vao);

		// Asegúrate de que el color se cargue desde el VBO de colores
		glBindBuffer(GL_ARRAY_BUFFER, meshData.colorVBO);
		glBindBuffer(GL_ARRAY_BUFFER, meshData.textureVBO);
		glEnableVertexAttribArray(2); // Activar el atributo de color

		size_t offset = 0;
		for (size_t i = 0; i < meshData.triangles.size(); i++) {
			const auto& triangle = meshData.triangles[i];
			//if (i % 6 == 0) glColor3f(0.6f, 0.6f, 0.6f); // Rojo
			//else if (i % 6 == 1) glColor3f(0.3f, 0.3f, 0.3f); // Verde
			//else if (i % 6 == 2) glColor3f(0.5f, 0.5f, 0.5f); // Azul
			//else if (i % 6 == 3) glColor3f(0.1f, 0.1f, 0.1f); // Amarillo
			//else if (i % 6 == 4) glColor3f(0.4f, 0.4f, 0.4f); // Magenta
			//else if (i % 6 == 5) glColor3f(0.2f, 0.2f, 0.2f); // Cian
			glDrawElements(GL_TRIANGLES, triangle.size(), GL_UNSIGNED_INT,
				(void*)(offset * sizeof(unsigned int)));
			offset += triangle.size();
		}
		glBindVertexArray(0);
	}
}

void LoadToBuffers(MeshData& meshData) 
{
	glGenVertexArrays(1, &meshData.vao);
	glGenBuffers(1, &meshData.vbo);
	glGenBuffers(1, &meshData.ebo);
	glGenBuffers(1, &meshData.normalVBO);
	glGenBuffers(1, &meshData.textureVBO);

	glBindVertexArray(meshData.vao);

	// Cargar vértices
	glBindBuffer(GL_ARRAY_BUFFER, meshData.vbo);
	glBufferData(GL_ARRAY_BUFFER, meshData.vertices.size() * sizeof(vec3),
		meshData.vertices.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_DOUBLE, GL_FALSE, sizeof(vec3), (void*)0);

	// Cargar normales
	if (!meshData.normals.empty()) {
		glBindBuffer(GL_ARRAY_BUFFER, meshData.normalVBO);
		glBufferData(GL_ARRAY_BUFFER, meshData.normals.size() * sizeof(vec3), meshData.normals.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(1); // Asegúrate de usar el índice correcto
		glVertexAttribPointer(1, 3, GL_DOUBLE, GL_FALSE, sizeof(vec3), (void*)0);
	}

	//Texturas
	if (!meshData.texCoords.empty()) {
		glGenBuffers(1, &meshData.textureVBO);
		glBindBuffer(GL_ARRAY_BUFFER, meshData.textureVBO);
		glBufferData(GL_ARRAY_BUFFER, meshData.texCoords.size() * sizeof(vec3), meshData.texCoords.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(2); // Atributo 2 para texturas
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_TRUE, sizeof(vec3), (void*)0); // Cambié a GL_FLOAT y 2 componentes
	}

	// Cargar índices
	vector<unsigned int> allIndices;
	for (const auto& triangle : meshData.triangles) {
		allIndices.insert(allIndices.end(), triangle.begin(), triangle.end());
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshData.triangles.size() * sizeof(unsigned int) * 3,
		allIndices.data(), GL_STATIC_DRAW);


	glBindVertexArray(0);
}

void cleanupMeshData(MeshData& meshData) {
	glDeleteBuffers(1, &meshData.vbo);
	glDeleteBuffers(1, &meshData.ebo);
	glDeleteBuffers(1, &meshData.colorVBO);  // Nuevo: eliminar VBO de colores
	glDeleteBuffers(1, &meshData.textureVBO);
	glDeleteVertexArrays(1, &meshData.vao);
}

vector<MeshData> LoadFBX()
{
	const char* file = "C:/Users/adriarj/Downloads/putin.fbx"; // Ruta del fitxer a carregar
	const struct aiScene* scene = aiImportFile(file,
		aiProcess_Triangulate | aiProcess_GenNormals);
	const float scaleFactor = 1.0f;
	if (!scene) {
		fprintf(stderr, "Error en carregar el fitxer: %s\n", aiGetErrorString());
		return {};
	}

	vector<MeshData> MayaTotal;

	for (unsigned int i = 0; i < scene->mNumMeshes; i++) 
	{
		aiMesh* mesh = scene->mMeshes[i];
		MeshData meshData;

		// Vèrtexs
		for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
			aiVector3D vertex = mesh->mVertices[v];
			meshData.vertices.push_back(vec3(
				vertex.x * scaleFactor,
				vertex.y * scaleFactor,
				vertex.z * scaleFactor));
			if (mesh->HasNormals()) {
				aiVector3D normal = mesh->mNormals[v];
				meshData.normals.push_back(vec3(normal.x, normal.y, normal.z));
			}
			// Coordenadas de textura (si están disponibles)
			if (mesh->HasTextureCoords(0)) {
				aiVector3D texCoord = mesh->mTextureCoords[0][v];
				meshData.texCoords.push_back(vec3(texCoord.x, texCoord.y, 0));
			}
		}

		// Índexs de triangles (3 per triangle)
		for (unsigned int f = 0; f < mesh->mNumFaces; f++) 
		{
			aiFace face = mesh->mFaces[f];
			if (face.mNumIndices != 3) {
				printf("Advertencia: Face %u no es un triángulo (tiene %u índices)\n",
					f, face.mNumIndices);
				continue;
			}
			vector<unsigned int> indices;
			for (unsigned int j = 0; j < face.mNumIndices; j++) {
				indices.push_back(face.mIndices[j]);
			}
			meshData.triangles.push_back(indices);
		}

		LoadToBuffers(meshData);
		MayaTotal.push_back(move(meshData));
	}
	aiReleaseImport(scene);
	return MayaTotal;

}
void LoadText()
{
	const char* Path = "C:/Users/adriarj/Downloads/putinText.png";
	ILuint imageID;
	ilGenImages(1, &imageID);
	ilBindImage(imageID);

	if (!ilLoadImage((const wchar_t*)Path)) {  // Cargamos la imagen usando la ruta
		ilDeleteImages(1, &imageID);
		return; // Si falla, terminamos aquí
	}

	ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
	int width = ilGetInteger(IL_IMAGE_WIDTH);
	int height = ilGetInteger(IL_IMAGE_HEIGHT);
	unsigned char* imageData = ilGetData();
	GLuint textureID;

	//ilLoadImage((const wchar_t* )textureID);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
	ilDeleteImages(1, &imageID);
}

void drawGrid(float size = 10.0f, int divisions = 10) {
	float step = size / divisions;
	float half = size / 2.0f;

	glBegin(GL_LINES);
	glColor3f(0.6f, 0.6f, 0.6f); // Color gris para la cuadrícula

	// Líneas paralelas al eje X
	for (int i = 0; i <= divisions; ++i) {
		float position = -half + i * step;
		glVertex3f(position, 0.0f, -half); // Línea desde (x, 0, -half)
		glVertex3f(position, 0.0f, half);  // hasta (x, 0, half)
	}

	// Líneas paralelas al eje Z
	for (int i = 0; i <= divisions; ++i) {
		float position = -half + i * step;
		glVertex3f(-half, 0.0f, position); // Línea desde (-half, 0, z)
		glVertex3f(half, 0.0f, position);  // hasta (half, 0, z)
	}

	glEnd();
}
vector<MeshData> dato;
float rotationX = 0.0f;  // Rotación alrededor del eje X
float rotationY = 0.0f;  // Rotación alrededor del eje Y
float objX = 0.0f;
float objY = 0.0f;
bool isDragging = false;  // Indica si el mouse está siendo arrastrado
bool isScrolling = false;
bool moveObject = false;
int lastMouseX, lastMouseY; // Última posición del mouse
float zoomLevel = -5.0f;
float cameraOffsetY;
float cameraOffsetX;



static void display_func() //funcion que se llama en el main, seria como un Update
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Actualizar la matriz de vista: zoom (eje Z) y desplazamiento en el eje Y (para mover la cámara arriba/abajo)	
	viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(cameraOffsetX * 0.4f, -cameraOffsetY * 0.4f, zoomLevel));
	viewMatrix = glm::rotate(viewMatrix, glm::radians(rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
	viewMatrix = glm::rotate(viewMatrix, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0));

	modelMatrix = glm::mat4(1.0f);	

	//drawGrid();
	// Crear la matriz de modelo: rotación del objeto en los ejes X e Y
	//modelMatrix = glm::mat4(1.0f);
	//modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationX), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotación en X
	//modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotación en Y

	// Multiplicación de las matrices: proyección * vista * modelo
	//glm::mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;

	// Aplicar la matriz MVP en OpenGL
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(glm::value_ptr(projectionMatrix));

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(glm::value_ptr(viewMatrix));

	drawModel(dato);
}

static bool processEvents() //funcion que gestion de eventos(mouse)
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		ImGui_ImplSDL2_ProcessEvent(&event);
		switch (event.type) {
		case SDL_QUIT:
			return false;
		case SDL_MOUSEBUTTONDOWN: //cuando se presiona el boton del mouse
			if (event.button.button == SDL_BUTTON_RIGHT) {//boton derecho
				isDragging = true;
				SDL_GetMouseState(&lastMouseX, &lastMouseY);
			}
			if (event.button.button == SDL_BUTTON_LEFT) // boton izquierdo + alt
			{
				if (SDL_GetModState() & KMOD_ALT)
				{
					isScrolling = true;
					SDL_GetMouseState(&lastMouseX, &lastMouseY);
				}
				else
				{
					moveObject = true;
					SDL_GetMouseState(&lastMouseX, &lastMouseY);
				}
			}
			break;
		case SDL_MOUSEBUTTONUP: //cuando se suelta el boton del mouse
			if (event.button.button == SDL_BUTTON_RIGHT && isDragging) {
				isDragging = false;
			}
			if ( event.button.button == SDL_BUTTON_LEFT && isScrolling)
			{
				isScrolling = false;
				moveObject = false;
			}
			break;
		case SDL_MOUSEWHEEL:
			// Ajustar el zoom basado en el movimiento de la rueda
			zoomLevel += event.wheel.y * 0.5f;
			// Limitar el zoom para evitar que se acerque o aleje demasiado
			zoomLevel = glm::clamp(zoomLevel, -20.0f, -1.0f);
			break;
		case SDL_MOUSEMOTION:
			if (isDragging || isScrolling || moveObject) {
				int mouseX, mouseY;
				SDL_GetMouseState(&mouseX, &mouseY);
				int deltaX = mouseX - lastMouseX;
				int deltaY = mouseY - lastMouseY;

				if (isDragging) {rotationY += deltaX * 0.5f; rotationX += deltaY * 0.5f;}
				if (isScrolling) { cameraOffsetY += deltaY * 0.05f; cameraOffsetX += deltaX * 0.05f;}

				lastMouseX = mouseX;
				lastMouseY = mouseY;
			}
			break;
		}
	}
	return true;
}

int main(int argc, char** argv) {
	MyWindow window("SDL2 Simple Example", WINDOW_SIZE.x, WINDOW_SIZE.y);
	init_openGL();
	srand(static_cast<unsigned int>(time(nullptr)));

	dato = LoadFBX(); // Cargar los vértices solo una vez
	LoadText();
	while (processEvents()) {
		const auto t0 = hrclock::now();
		display_func();
		window.draw();
		const auto t1 = hrclock::now();
		const auto dt = t1 - t0;
		if(dt<FRAME_DT) this_thread::sleep_for(FRAME_DT - dt);
	}
	for (auto& mesh : dato) {
		cleanupMeshData(mesh);
	}

	return 0;
}