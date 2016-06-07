#ifndef _MESH_RENDERER_H
#define _MESH_RENDERER_H

#define GLEW_STATIC
#include <GL/glew.h>
// GLFW
#include <GLFW/glfw3.h>
#include "Shader.h"
// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext.hpp>

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include "tiny_obj_loader.h"

#include <vector>

class MeshRenderer {
public:
	MeshRenderer(Shader* shader, GLuint texture);
	~MeshRenderer();
	Shader* shader;
	void render(glm::mat4 &transform, glm::mat4 &model, glm::mat4 &view, glm::mat4 &projection);
	void initialize(std::string filename);
private:
	unsigned int numShapes;
	std::vector<GLuint> vertexPosBuffers, vertexTexCoordsBuffers, elementBuffers, vertexArrayBuffers;
	//GLuint vertexPosBuffer, vertexTexCoordsBuffer, elementBuffer, vertexArrayBuffer, texture;
	GLuint texture;
	GLint modelLoc, viewLoc, projectionLoc, scaleLoc;
	std::vector<unsigned int> numElements;
};

MeshRenderer::MeshRenderer(Shader* shader, GLuint texture) {
	this->shader = shader;
	this->texture = texture;
	modelLoc = glGetUniformLocation(shader->Program, "model");
	viewLoc = glGetUniformLocation(shader->Program, "view");
	projectionLoc = glGetUniformLocation(shader->Program, "projection");
	scaleLoc = glGetUniformLocation(shader->Program, "scale");
}

void MeshRenderer::initialize(std::string filename) {
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	if(!tinyobj::LoadObj(shapes, materials, err, filename.c_str())) {
		std::cerr << err << std::endl;
	}
	numShapes = shapes.size();
	numShapes = 1;
	for (int i = 0; i < numShapes; i++) {
		GLuint vertexPosBuffer, vertexTexCoordsBuffer, elementBuffer, vertexArrayBuffer;
		// create the vertex buffer
		glGenBuffers(1, &vertexPosBuffer);
		glGenBuffers(1, &vertexTexCoordsBuffer);
		glGenBuffers(1, &elementBuffer);
		glGenVertexArrays(1, &vertexArrayBuffer);
		// bind the vertex array object
		glBindVertexArray(vertexArrayBuffer);

		// bind the vertex position buffer
		glBindBuffer(GL_ARRAY_BUFFER, vertexPosBuffer);
		glBufferData(GL_ARRAY_BUFFER, shapes[i].mesh.positions.size() * sizeof(float), &(shapes[i].mesh.positions[0]), GL_STATIC_DRAW);

		// bind the vertex texture buffer
		glBindBuffer(GL_ARRAY_BUFFER, vertexTexCoordsBuffer);
		glBufferData(GL_ARRAY_BUFFER, shapes[i].mesh.texcoords.size() * sizeof(float), &(shapes[i].mesh.texcoords[0]), GL_STATIC_DRAW);

		// bind the element buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapes[i].mesh.indices.size() * sizeof(unsigned int), &(shapes[i].mesh.indices[0]), GL_STATIC_DRAW);

		// specify how the attributes should be parsed
		glBindBuffer(GL_ARRAY_BUFFER, vertexPosBuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*) 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, vertexTexCoordsBuffer);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*) 0);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// unbind the array object
		glBindVertexArray(0);
		numElements[i] = shapes[0].mesh.indices.size();
		vertexPosBuffers.push_back(vertexPosBuffer);
		vertexTexCoordsBuffers.push_back(vertexTexCoordsBuffer);
		elementBuffers.push_back(elementBuffer);
		vertexArrayBuffers.push_back(vertexArrayBuffer);
	}
}

void MeshRenderer::render(glm::mat4 &transform, glm::mat4 &model, glm::mat4 &view, glm::mat4 &projection) {
	shader->Use();
	glBindTexture(GL_TEXTURE_2D, texture);
	for (int i = 0; i < numShapes; i++) {
		glBindVertexArray(vertexArrayBuffers[i]);
		glUniformMatrix4fv(scaleLoc, 1, GL_FALSE, glm::value_ptr(transform));
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, numElements[i], GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}
}

MeshRenderer::~MeshRenderer() {
	for (int i = 0; i < numShapes; i++) {
		glDeleteVertexArrays(1, &vertexArrayBuffers[i]);
		glDeleteBuffers(1, &vertexPosBuffers[i]);
		glDeleteBuffers(1, &vertexTexCoordsBuffers[i]);
		glDeleteBuffers(1, &elementBuffers[i]);
	}
}

#endif
