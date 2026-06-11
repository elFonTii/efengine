#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aUV;
layout (location = 3) in vec3 aTangent;

/*
Cada vertice de un modelo tiene un vector Normal (hacia dónde mira )
una vector Tangente (hacia dónde va la textura en X) y un Bitangente (hacia dónde va la textura en Y). 
A esto se le llama la Matriz TBN.
*/

out vec3 vFragPos;
out vec3 vUV;
out mat3 vTBN;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

