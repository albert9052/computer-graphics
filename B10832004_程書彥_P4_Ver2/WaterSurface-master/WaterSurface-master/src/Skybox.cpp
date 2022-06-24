#include "Skybox.h"

Skybox::Skybox(unsigned int cubemapTexture) {

	for (int i = 0; i < 3 * 6 * 6; i++) {

		skyboxIndices.push_back(i);
	}

	this->cubemapTexture = cubemapTexture;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);	

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, skyboxVertices.size() * sizeof(glm::vec3), &skyboxVertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, skyboxIndices.size() * sizeof(unsigned int), &skyboxIndices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	glBindVertexArray(0);
}

void Skybox::Draw(Shader& shader) {

	glDepthMask(GL_FALSE);
	glBindVertexArray(VAO);
	glUniform1i(glGetUniformLocation(shader.Program, "skybox"), 3);
	glActiveTexture(GL_TEXTURE0 + 3);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	//glActiveTexture(GL_TEXTURE0 + 6);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthMask(GL_TRUE);
}
