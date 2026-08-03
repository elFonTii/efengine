#version 450 core
in vec3 vFragPos;
in vec3 vNormal;
in vec2 vUV;

out vec4 FragColor;

layout(std140, binding = 0) uniform Frame {
    mat4 uView;
    mat4 uProjection;
    mat4 uLightSpaceMatrix;
    mat4 uInvViewProjRot;
    vec4 uViewPos;
    vec4 uShadowParams;
    vec4 uIblParams;
};

layout(std140, binding = 1) uniform Lights {
    vec4  uLightPositions[4];
    vec4  uLightColors[4];
    vec4  uLightDir;
    vec4  uDirLightColor;
    ivec4 uLightCounts;
};

layout(binding = 0) uniform sampler2D uAlbedoMap;

void main() {
    vec3 albedo = texture(uAlbedoMap, vUV).rgb;

    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPositions[0].xyz - vFragPos);   // del fragmento hacia la luz
    vec3 V = normalize(uViewPos.xyz - vFragPos);    // del fragmento hacia la cámara

    // Ambiente: término constante para que las sombras no queden negras.
    float ambientStrength = 0.1;
    vec3  ambient = ambientStrength * uLightColors[0].rgb;

    // Difuso: ley del coseno (Lambert).
    float diff    = max(dot(N, L), 0.0);
    vec3  diffuse = diff * uLightColors[0].rgb;

    // Especular (Phong clásico: reflejo de L respecto a N, visto desde V).
    float specularStrength = 0.5;
    float shininess        = 32.0;
    vec3  R    = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), shininess);
    vec3  specular = specularStrength * spec * uLightColors[0].rgb;

    vec3 color = (ambient + diffuse + specular) * albedo;
    // Corrección gamma: la textura albedo es sRGB (se linealiza al muestrear).
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}