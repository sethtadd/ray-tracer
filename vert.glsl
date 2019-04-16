#version 430 core

layout (location = 0) in vec2 vert;
layout (location = 1) in vec2 texCoordIn;

/* Write interpolated texture coordinate to fragment shader */
out vec2 texCoord;

void main() {
  gl_Position = vec4(vert, 0.0, 1.0);
  texCoord = texCoordIn;
}
