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

#include "common/lights.glsl"

layout(binding = 0) uniform sampler2D uAlbedoMap;

void main() {
    vec3 albedo = texture(uAlbedoMap, vUV).rgb;

    vec3 N = normalize(vNormal);
    vec3 V = normalize(uViewPos.xyz - vFragPos);

    // Phong ilumina con una sola luz: la primera local, si no la primera direccional.
    vec3 L          = N;
    vec3 lightColor = vec3(0.0);
    if (uLightCounts.x > 0u) {
        L          = normalize(uLocalLights[0].positionRange.xyz - vFragPos);
        lightColor = uLocalLights[0].colorRadius.rgb;
    } else if (uLightCounts.y > 0u) {
        L          = normalize(-uDirDirection[0].xyz);
        lightColor = uDirColor[0].rgb;
    }

    // Ambiente: término constante para que las sombras no queden negras.
    float ambientStrength = 0.1;
    vec3  ambient = ambientStrength * lightColor;

    // Difuso: ley del coseno (Lambert).
    float diff    = max(dot(N, L), 0.0);
    vec3  diffuse = diff * lightColor;

    // Especular (Phong clásico: reflejo de L respecto a N, visto desde V).
    float specularStrength = 0.5;
    float shininess        = 32.0;
    vec3  R    = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), shininess);
    vec3  specular = specularStrength * spec * lightColor;

    vec3 color = (ambient + diffuse + specular) * albedo;
    // Corrección gamma: la textura albedo es sRGB (se linealiza al muestrear).
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}