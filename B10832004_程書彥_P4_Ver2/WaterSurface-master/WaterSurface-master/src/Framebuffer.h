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

class Framebuffer {

public:

	Framebuffer(float width, float height);
	void updateSize(float width, float height);
	void clear();
	void use();

	unsigned int framebuffer;
	unsigned int texColorBuffer;
	unsigned int rbo;

private:

};
