#include "Mesh.h"

Mesh::Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture2D> textures)
{
	this->vertices = vertices;
	this->indices = indices;
	this->textures = textures;

	setupMesh();
}

void Mesh::setupMesh() {

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	// vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	
	// vertex normals
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

	// vertex texture coords
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

	glBindVertexArray(0);
}

void Mesh::Draw(Shader& shader) {

	unsigned int diffuseNr = 1;
	unsigned int specularNr = 1;
	if (textures.size() >= 2) {

		for (int i = 0; i < textures.size(); i++) {

			glActiveTexture(GL_TEXTURE0 + i);
			//string number;
			//Texture2D::Type name = textures[i].type;
			//if (name == Texture2D::Type::TEXTURE_DIFFUSE) {

			//	number = std::to_string(diffuseNr++);
			//}
			//else if (name == Texture2D::Type::TEXTURE_SPECULAR) {

			//	number = std::to_string(specularNr++);
			//}

			//glUniform1f(glGetUniformLocation(shader.Program, ("material." + name + number).c_str()), i);
			glUniform1i(glGetUniformLocation(shader.Program, ("u_texture_" + std::to_string(i)).c_str()), i);
			glBindTexture(GL_TEXTURE_2D, textures[i].id);
		}
		glActiveTexture(GL_TEXTURE0);
	}
	else {

		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(shader.Program, "u_texture_0"), 0);
		glBindTexture(GL_TEXTURE_2D, textures[0].id);
		glActiveTexture(GL_TEXTURE0);
	}
	if (textures.size() >= 2) {

		glUniform1f(glGetUniformLocation(shader.Program, "hasHeightMap"), 1.0f);
	}
	else {

		glUniform1f(glGetUniformLocation(shader.Program, "hasHeightMap"), 0.0f);
	}

	// Draw mesh
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

}