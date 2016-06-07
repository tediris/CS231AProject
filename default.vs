#version 330 core
layout (location = 0) in vec3 position;
out vec3 vertexColor;
void main() {
	gl_Position = vec4(position, 1.0);
	vertexColor = vec3(0.8f, 0.0f, 0.7f);
}
