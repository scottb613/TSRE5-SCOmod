#version 130

attribute vec4 vertex;
attribute vec3 normal;
attribute vec2 aTextureCoord;

uniform float lod;
uniform mat4 uPMatrix;
uniform mat4 uShadowPMatrix;
uniform mat4 uMVMatrix;
uniform mat4 uMSMatrix;
uniform bool uUseForestOffsets;
attribute vec3 forestOffset;

out vec2 vTextureCoord;

void main() {
    vec4 localVertex = vertex;
    if (uUseForestOffsets)
        localVertex.xyz += forestOffset;
    gl_Position = uShadowPMatrix * uMVMatrix * uMSMatrix * localVertex;
    vTextureCoord = aTextureCoord;
}
