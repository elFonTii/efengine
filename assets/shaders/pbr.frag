#version 330 core
in vec3 vFragPos;
in vec2 vUV;
in mat3 vTBN;
out vec4 FragColor;

uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform vec3 uViewPos;

void main() {
    vec3  objectColor = vec3(1.0, 0.5, 0.31); 

    vec3 N = normalize(vTBN[2]);               // normal (indice 2 de la TBN) 
    vec3 L = normalize(uLightPos - vFragPos);  // del fragmento HACIA la luz
    vec3 V = normalize(uViewPos - vFragPos);   // del fragmento HACIA la cámara

    float ambientStrength = 0.1;
    vec3  ambient = ambientStrength * uLightColor;

    float diff    = max(dot(N, L), 0.0);
    vec3  diffuse = diff * uLightColor;

    float specularStrength = 0.5;
    float shininess        = 32.0;
    vec3  R    = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), shininess);
    vec3  specular = specularStrength * spec * uLightColor;

    FragColor = vec4((ambient + diffuse + specular) * objectColor, 1.0);
}