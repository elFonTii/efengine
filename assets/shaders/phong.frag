#version 450 core
in vec3 vFragPos;
in vec3 vNormal;
in vec2 vUV;

out vec4 FragColor;

uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform vec3 uViewPos;

uniform sampler2D uAlbedoMap;

void main() {
    vec3 albedo = texture(uAlbedoMap, vUV).rgb;

    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vFragPos);   // del fragmento hacia la luz
    vec3 V = normalize(uViewPos - vFragPos);    // del fragmento hacia la cámara

    // Ambiente: término constante para que las sombras no queden negras.
    float ambientStrength = 0.1;
    vec3  ambient = ambientStrength * uLightColor;

    // Difuso: ley del coseno (Lambert).
    float diff    = max(dot(N, L), 0.0);
    vec3  diffuse = diff * uLightColor;

    // Especular (Phong clásico: reflejo de L respecto a N, visto desde V).
    float specularStrength = 0.5;
    float shininess        = 32.0;
    vec3  R    = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), shininess);
    vec3  specular = specularStrength * spec * uLightColor;

    vec3 color = (ambient + diffuse + specular) * albedo;
    // Corrección gamma: la textura albedo es sRGB (se linealiza al muestrear).
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}