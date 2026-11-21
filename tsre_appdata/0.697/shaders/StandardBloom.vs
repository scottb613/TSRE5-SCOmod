#version 130

attribute vec4 vertex;
attribute vec3 normal;
attribute vec2 aTextureCoord;
attribute float alpha;

uniform float lod;
uniform mat4 uShadowPMatrix;
uniform mat4 uShadow2PMatrix;
uniform mat4 uPMatrix;
uniform mat4 uFMatrix;
uniform mat4 uMVMatrix;
uniform mat4 uMSMatrix;
uniform bool uUseForestOffsets;
attribute vec3 forestOffset;
uniform float fogDensity;

varying vec2 vTextureCoord;
varying float fogFactor;
varying vec3 vNormal;
varying vec4 shadowPos;
varying vec4 shadow2Pos;
varying float vAlpha;

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
