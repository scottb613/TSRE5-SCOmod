#version 330 core

in vec4 vertex;
in vec3 normal;
in vec2 aTextureCoord;

uniform float lod;
uniform mat4 uPMatrix;
uniform mat4 uShadowPMatrix;
uniform mat4 uMVMatrix;
uniform mat4 uMSMatrix;
uniform bool uUseForestOffsets;
in vec3 forestOffset;

out vec2 vTextureCoord;

void main() {
    vec4 localVertex = vertex;
    if (uUseForestOffsets)
        localVertex.xyz += forestOffset;
    gl_Position = uShadowPMatrix * uMVMatrix * uMSMatrix * localVertex;
    vTextureCoord = aTextureCoord;
}
