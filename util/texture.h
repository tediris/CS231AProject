#ifndef _TEXTURE_H
#define _TEXTURE_H

// GLEW
#define GLEW_STATIC
#include <GL/glew.h>
// GLFW
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLuint loadTexture(std::string filename) {
	// load an image
	int w;
	int h;
	int comp;
	unsigned char* image = stbi_load(filename.c_str(), &w, &h, &comp, STBI_rgb);
	if(image == nullptr) {
		throw(std::string("Failed to load texture"));
		std::cerr << "FAILED TO LOAD TEXTURE" << std::endl;
		return -1;
	}
	GLuint texture;
	glGenTextures(1, &texture);
	// bind it so that texture commands are applied
	glBindTexture(GL_TEXTURE_2D, texture);
	// Set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_LINEAR);	// Set texture wrapping to GL_REPEAT (usually basic wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_LINEAR);
	// Set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load the image into the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	// unbind the texture
	glBindTexture(GL_TEXTURE_2D, 0);
	// free the image
	stbi_image_free(image);
	return texture;
}

#endif
