#include <GL/glew.h>
#include <chrono>
#include <thread>
#include <exception>
#include <glm/glm.hpp>
#include "MyWindow.h"
#include "stdio.h"
#include "assimp/cimport.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <GL/gl.h>
#include <GL/glu.h>
#include <IL/il.h>       // DevIL para la carga de imágenes
#include <IL/ilu.h>      // Funciones adicionales de DevIL
#include <IL/ilut.h>     // Para integración con OpenGL

#define CHECKERS_WIDTH 64
#define CHECKERS_HEIGHT 64

GLubyte checkerImage[CHECKERS_HEIGHT][CHECKERS_WIDTH][4];
GLuint textureID;

using namespace std;

using hrclock = chrono::high_resolution_clock;
using u8vec4 = glm::u8vec4;
using ivec2 = glm::ivec2;
using vec3 = glm::dvec3;

static const ivec2 WINDOW_SIZE(512, 512);
static const unsigned int FPS = 60;
static const auto FRAME_DT = 1.0s / FPS;

vector<float> vertices;
vector<float> uvs;
vector<unsigned int> indices;

float modelScale = 1.0f; // Factor de escala global para el modelo
vec3 modelCenter(0.0f); // Centro del modelo para su ajuste en escena

MyWindow myWindow("SDL2 Simple Example", WINDOW_SIZE.x, WINDOW_SIZE.y);

// Función para configurar la proyección
void setupProjection(float fov, float aspectRatio, float nearPlane, float farPlane) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov, aspectRatio, nearPlane, farPlane);
}

// Función para configurar la vista de la cámara
void setupView() {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float eyeX = myWindow.cameraDistance * sin(myWindow.cameraAngleY) * cos(myWindow.cameraAngleX) + myWindow.panX;
    float eyeY = myWindow.cameraDistance * sin(myWindow.cameraAngleX) + myWindow.panY;
    float eyeZ = myWindow.cameraDistance * cos(myWindow.cameraAngleY) * cos(myWindow.cameraAngleX);

    gluLookAt(eyeX, eyeY, eyeZ, myWindow.panX, myWindow.panY, 0.0, 0.0, 1.0, 0.0);
}

// Inicialización de OpenGL
static void init_openGL() {
    glewInit();
    if (!GLEW_VERSION_3_0) throw std::exception("OpenGL 3.0 API is not available.");
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.5, 0.5, 0.5, 1.0);

    float fov = 45.0f;
    float aspectRatio = static_cast<float>(WINDOW_SIZE.x) / static_cast<float>(WINDOW_SIZE.y);
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    setupProjection(fov, aspectRatio, nearPlane, farPlane);
}

// Generar textura checkerboard
void generateCheckerImage() {
    for (int i = 0; i < CHECKERS_HEIGHT; i++) {
        for (int j = 0; j < CHECKERS_WIDTH; j++) {
            int c = (((i & 0x8) == 0) ^ ((j & 0x8) == 0)) * 255;
            checkerImage[i][j][0] = (GLubyte)c;
            checkerImage[i][j][1] = (GLubyte)c;
            checkerImage[i][j][2] = (GLubyte)c;
            checkerImage[i][j][3] = (GLubyte)255;
        }
    }
}

GLuint loadTexture(const char* texturePath) {
    ILuint imageID;
    GLuint textureID;

    ilGenImages(1, &imageID);
    ilBindImage(imageID);

    if (!ilLoadImage((const wchar_t*)texturePath)) {
        ilDeleteImages(1, &imageID);
        fprintf(stderr, "No se pudo cargar la textura desde: %s\n", texturePath);
        return 0; // Retorna 0 si falla la carga
    }

    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE); // Convertir la imagen a formato RGBA

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ilGetInteger(IL_IMAGE_WIDTH),
        ilGetInteger(IL_IMAGE_HEIGHT), 0, GL_RGBA, GL_UNSIGNED_BYTE, ilGetData());

    ilDeleteImages(1, &imageID); // Liberar la imagen en DevIL
    return textureID;
}

// Configurar la textura en OpenGL
void drawTextures(const char* texturePath) {
    textureID = loadTexture(texturePath); // Cargar textura desde la ruta dada

    if (textureID == 0) {
        fprintf(stderr, "Fallo en la carga de la textura. Se usará una textura checkerboard predeterminada.\n");
        generateCheckerImage();  // Genera checkerboard si la carga falla

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CHECKERS_WIDTH, CHECKERS_HEIGHT, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, checkerImage);
    }
}

// Ajusta el centro del modelo en el origen
void centerModel(const aiScene* scene) {
    aiVector3D min, max;
    min = max = scene->mMeshes[0]->mVertices[0];

    // Encuentra los límites del modelo
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        for (unsigned int v = 0; v < scene->mMeshes[i]->mNumVertices; v++) {
            aiVector3D vertex = scene->mMeshes[i]->mVertices[v];
            min.x = min(min.x, vertex.x);
            min.y = min(min.y, vertex.y);
            min.z = min(min.z, vertex.z);
            max.x = max(max.x, vertex.x);
            max.y = max(max.y, vertex.y);
            max.z = max(max.z, vertex.z);
        }
    }
    modelCenter = vec3((min.x + max.x) / 2, (min.y + max.y) / 2, (min.z + max.z) / 2);
    modelScale = 2.0f / length(vec3(max.x - min.x, max.y - min.y, max.z - min.z));
}

// Modifica drawFBX para centrar y escalar el modelo
int drawFBX() {
    const char* file = "C:/Users/didacpm/Downloads/blake.fbx";
    const aiScene* scene = aiImportFile(file, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenUVCoords);

    if (!scene) {
        fprintf(stderr, "Error al cargar el archivo: %s\n", aiGetErrorString());
        return -1;
    }

    centerModel(scene); // Centrar el modelo

    vertices.clear();
    uvs.clear();
    indices.clear();

    // Procesar mallas del modelo
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];

        // Procesar vértices y coordenadas UV
        for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
            aiVector3D vertex = mesh->mVertices[v];
            vertices.push_back((vertex.x - modelCenter.x) * modelScale);
            vertices.push_back((vertex.y - modelCenter.y) * modelScale);
            vertices.push_back((vertex.z - modelCenter.z) * modelScale);

            if (mesh->HasTextureCoords(0)) {
                aiVector3D uv = mesh->mTextureCoords[0][v];
                uvs.push_back(uv.x);
                uvs.push_back(uv.y);
            }
            else {
                uvs.push_back(0.0f);
                uvs.push_back(0.0f);
            }
        }

        // Procesar índices de caras
        for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
            aiFace face = mesh->mFaces[f];
            if (face.mNumIndices == 3) {
                indices.push_back(face.mIndices[0]);
                indices.push_back(face.mIndices[1]);
                indices.push_back(face.mIndices[2]);
            }
        }
    }

    aiReleaseImport(scene);
    return 0;
}

// Modificar display_func para usar la escala y posicionamiento
static void display_func() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setupView();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureID);  // Asegurarse de que textureID esté enlazado

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, vertices.data());
    glTexCoordPointer(2, GL_FLOAT, 0, uvs.data());
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, indices.data());

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_TEXTURE_2D);
}

int main(int argc, char** argv) {
    ilInit();
    iluInit();
    ilutRenderer(ILUT_OPENGL); // Configura DevIL para OpenGL
    init_openGL();

    drawTextures("C:/Users/didacpm/Downloads/blakeTex.png");  // Cargar y configurar textura desde ruta

    if (drawFBX() == -1) {
        fprintf(stderr, "No se pudo cargar el modelo FBX\n");
        return -1;
    }

    while (myWindow.processEvents() && myWindow.isOpen()) {
        const auto t0 = hrclock::now();
        display_func();
        myWindow.draw();
        myWindow.swapBuffers();
        const auto t1 = hrclock::now();
        const auto dt = t1 - t0;
        if (dt < FRAME_DT) this_thread::sleep_for(FRAME_DT - dt);
    }

    return 0;
}