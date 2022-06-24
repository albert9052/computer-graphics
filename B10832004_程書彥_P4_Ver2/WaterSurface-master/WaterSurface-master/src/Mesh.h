#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "RenderUtilities/Shader.h"
#include "RenderUtilities/Texture.h"
#include "Utilities/Pnt3f.H"

#include <string>
#include <vector>

using std::string;
using std::vector;

struct Vertex {

	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;
};

class Mesh {

public:

	vector<Vertex> vertices;
	vector<unsigned int> indices;
	vector<Texture2D> textures;

	Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture2D> textures);
	void Draw(Shader& shader);

private:

	unsigned int VAO, VBO, EBO;
	void setupMesh();

};