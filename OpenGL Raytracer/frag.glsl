#version 430 core
/* The fragment color */
layout (location = 0) out vec4 fColor;

/* This comes interpolated from the vertex shader */
in vec2 texCoord;

/* The texture we are going to sample */
uniform sampler2D tex;

void main() {
  /* Well, simply sample the texture */
  fColor = texture(tex, texCoord);
}
