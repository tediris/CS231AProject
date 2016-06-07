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
	void render(glm::mat4 &transform, glm::mat4 &scale, glm::mat4 &model, glm::mat4 &view, glm::mat4 &projection);
	//void initialize(std::vector<float> vertexPos, std::vector<float> vertexTexCoords, std::vector<unsigned int> indices);
	void initialize(std::string filename);
private:
	std::vector<GLuint> vertexPosBuffers, vertexTexCoordsBuffers, elementBuffers, vertexArrayBuffers;
	//GLuint vertexPosBuffer, vertexTexCoordsBuffer, elementBuffer, vertexArrayBuffer, texture;
	GLuint texture;
	GLint modelLoc, viewLoc, projectionLoc, scaleLoc, transformLoc;
	unsigned int numElements;
	void loadOBJ(std::string filename);
	unsigned int numShapes;
	std::vector<unsigned int> elemsPerShape;
};

MeshRenderer::MeshRenderer(Shader* shader, GLuint texture) {
	this->shader = shader;
	this->texture = texture;
	modelLoc = glGetUniformLocation(shader->Program, "model");
	//viewLoc = glGetUniformLocation(shader->Program, "view");
	projectionLoc = glGetUniformLocation(shader->Program, "projection");
	scaleLoc = glGetUniformLocation(shader->Program, "scale");
	transformLoc = glGetUniformLocation(shader->Program, "transform");
}

//void MeshRenderer::initialize(std::vector<float> vertexPos, std::vector<float> vertexTexCoords, std::vector<unsigned int> indices) {
void MeshRenderer::initialize(std::string filename) {
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	if(!tinyobj::LoadObj(shapes, materials, err, filename.c_str())) {
		std::cerr << err << std::endl;
	}
	std::cout << "# of shapes    : " << shapes.size() << std::endl;
	numShapes = shapes.size();
	for (int i = 0; i < numShapes; i++) {
		std::vector<float> vertexPos = shapes[i].mesh.positions;
		std::vector<float> vertexTexCoords = shapes[i].mesh.texcoords;
		std::vector<unsigned int> indices = shapes[i].mesh.indices;

		GLuint vertexPosBuffer, vertexTexCoordsBuffer, elementBuffer, vertexArrayBuffer;
		// create the vertex buffer
		glGenBuffers(1, &vertexPosBuffer);
		glGenBuffers(1, &vertexTexCoordsBuffer);
		glGenBuffers(1, &elementBuffer);
		glGenVertexArrays(1, &vertexArrayBuffer);
		std::cout << "Created vertex array " << vertexArrayBuffer << std::endl;
		// bind the vertex array object
		glBindVertexArray(vertexArrayBuffer);

		// bind the vertex position buffer
		glBindBuffer(GL_ARRAY_BUFFER, vertexPosBuffer);
		//glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
		glBufferData(GL_ARRAY_BUFFER, vertexPos.size() * sizeof(float), &(vertexPos[0]), GL_STATIC_DRAW);

		// bind the vertex texture buffer
		glBindBuffer(GL_ARRAY_BUFFER, vertexTexCoordsBuffer);
		glBufferData(GL_ARRAY_BUFFER, vertexTexCoords.size() * sizeof(float), &(vertexTexCoords[0]), GL_STATIC_DRAW);

		// bind the element buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &(indices[0]), GL_STATIC_DRAW);

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

		numElements = indices.size();
		elemsPerShape.push_back(numElements);
		vertexPosBuffers.push_back(vertexPosBuffer);
		vertexTexCoordsBuffers.push_back(vertexTexCoordsBuffer);
		elementBuffers.push_back(elementBuffer);
		vertexArrayBuffers.push_back(vertexArrayBuffer);
		std::cout << "pushed back " << vertexArrayBuffer << std::endl;
	}
	std::cout << "[";
	for (int i = 0; i < numShapes; i++) {
		std::cout << vertexArrayBuffers[i] << " ";
	}
	std::cout << "]" << std::endl;
}

void MeshRenderer::render(glm::mat4 &transform, glm::mat4 &scale, glm::mat4 &model, glm::mat4 &view, glm::mat4 &projection) {
	shader->Use();
	glBindTexture(GL_TEXTURE_2D, texture);
	for (int i = 0; i < numShapes; i++) {
		//std::cout << vertexArrayBuffers[i] << " ";
		glBindVertexArray(vertexArrayBuffers[i]);
		glUniformMatrix4fv(scaleLoc, 1, GL_FALSE, glm::value_ptr(scale));
		glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));
		//glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, numElements, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}
	//std::cout << std::endl;
}

MeshRenderer::~MeshRenderer() {
	// glDeleteVertexArrays(1, &vertexArrayBuffer);
	// glDeleteBuffers(1, &vertexPosBuffer);
	// glDeleteBuffers(1, &vertexTexCoordsBuffer);
	// glDeleteBuffers(1, &elementBuffer);
}

#endif
