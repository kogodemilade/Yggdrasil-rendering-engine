#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
// layout (location = 1) in vec2 aTexture;

// uniform float offSetX;
// uniform float offSetY;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 VertexColor;
// out vec3 pos;
// out vec2 TexCoord;

void main(){
    gl_Position = projection*view*model*vec4(aPos.x, aPos.y, aPos.z,  1.0);
    // pos = vec3(gl_Position.xyz); 
    VertexColor = vec3(aColor);
    // TexCoord = aTexture;
}