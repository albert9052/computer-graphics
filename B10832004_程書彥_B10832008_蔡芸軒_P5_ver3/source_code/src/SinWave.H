#ifndef SINWAVE_H
#define SINWAVE_H

#include <vector>
#include "RenderUtilities/BufferObject.h"
#include "RenderUtilities/Shader.h"
#include "RenderUtilities/Texture.h"
#include "RenderUtilities/Mesh.h"
#include "Global.H"
using std::vector;

class SinWave {
public:
	SinWave();
	void draw();
	void init();

	float t = 0;
	VAO* vao;
	VBO* vbo;
	TextureCubeMap* reflectSkybox = nullptr;

	Shader* shader = nullptr;
	GLfloat vertices[1800];
	vector<const GLchar*> fileNames;
	glm::mat4 model_matrix = glm::mat4(100, 0, 0, 0, 0, 1, 0, 0, 0, 0, 100, 0, 0, 50, 0, 1);
};

#endif