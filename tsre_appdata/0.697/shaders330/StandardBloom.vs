#version 330 core

in vec4 vertex;
in vec3 normal;
in vec2 aTextureCoord;
in float alpha;

uniform float lod;
uniform mat4 uShadowPMatrix;
uniform mat4 uShadow2PMatrix;
uniform mat4 uPMatrix;
uniform mat4 uFMatrix;
uniform mat4 uMVMatrix;
uniform mat4 uMSMatrix;
uniform bool uUseForestOffsets;
in vec3 forestOffset;
uniform float fogDensity;

out vec2 vTextureCoord;
out float fogFactor;
out vec3 vNormal;
out vec4 shadowPos;
out vec4 shadow2Pos;
out float vAlpha;

void main() {
    vec4 localVertex = vertex;
    if (uUseForestOffsets)
        localVertex.xyz += forestOffset;
    shadowPos = uShadowPMatrix * uMVMatrix * uMSMatrix * localVertex;
    shadow2Pos = uShadow2PMatrix * uMVMatrix * uMSMatrix * localVertex;
    gl_Position = uPMatrix * uMVMatrix * uMSMatrix * localVertex;
    vec4 fogPosition = uFMatrix * uMVMatrix * uMSMatrix * localVertex;
    vTextureCoord = aTextureCoord;

    fogFactor = sqrt((fogPosition.x)*(fogPosition.x) + (fogPosition.z)*(fogPosition.z))/(lod*1.4);
    fogFactor = clamp(fogFactor, 0.0, fogDensity);
    fogFactor = min(fogFactor, lod);
    fogFactor = abs(fogFactor);


    vNormal = normal;
    vAlpha = alpha;

}
