#version 330 core

in vec3 vFragPos;   // posición del fragmento en espacio mundo
in vec2 vUV;
in mat3 vTBN;       // base tangente→mundo (columna 2 = normal)

out vec4 FragColor;

// --- Luces puntuales (arreglo fijo) ---
#define MAX_LIGHTS 4
uniform vec3  uLightPositions[MAX_LIGHTS];
uniform vec3  uLightColors[MAX_LIGHTS];
uniform int   uLightCount;
uniform vec3  uViewPos;
uniform float uAmbientFactor;   // factor de luz ambiente (p. ej. 0.03)

// --- Albedo ---
uniform sampler2D uAlbedoMap;
uniform int       uHasAlbedoMap;
uniform vec3      uAlbedoTint;

// --- Metallic ---
uniform sampler2D uMetallicMap;
uniform int       uHasMetallicMap;
uniform float     uMetallic;

// --- Roughness ---
uniform sampler2D uRoughnessMap;
uniform int       uHasRoughnessMap;
uniform float     uRoughness;

const float PI = 3.14159265359;

// Normal Distribution Function (Trowbridge-Reitz GGX): qué fracción de las
// microfacetas apuntan hacia el half-vector H. Concentra el brillo especular.
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;

    return a2 / max(denom, 0.0000001);
}

// Geometría (Schlick-GGX): cuánta luz se auto-ensombrece/ocluye entre microfacetas.
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;     // remap para luz directa

    return NdotV / (NdotV * (1.0 - k) + k);
}

// Smith: combina la geometría vista desde la cámara (V) y desde la luz (L).
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggxV  = GeometrySchlickGGX(NdotV, roughness);
    float ggxL  = GeometrySchlickGGX(NdotL, roughness);

    return ggxV * ggxL;
}

// Fresnel-Schlick: cuánta luz se refleja según el ángulo de visión (más en rasante).
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    // --- Propiedades del material (textura si existe, escalar si no) ---
    vec3 albedo = (uHasAlbedoMap == 1)
                ? texture(uAlbedoMap, vUV).rgb * uAlbedoTint
                : uAlbedoTint;

    float metallic  = (uHasMetallicMap == 1)  ? texture(uMetallicMap, vUV).r  : uMetallic;
    float roughness = (uHasRoughnessMap == 1) ? texture(uRoughnessMap, vUV).r : uRoughness;

    vec3 N = normalize(vTBN[2]);               // normal geométrica (sin normal map todavía)
    vec3 V = normalize(uViewPos - vFragPos);   // del fragmento hacia la cámara

    // F0: reflectancia base con incidencia normal. Dieléctricos ≈ 0.04;
    // los metales reflejan con su propio color (albedo).
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // --- Luz directa: sumatoria Cook-Torrance sobre cada luz puntual ---
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < uLightCount; ++i) {
        vec3  L = normalize(uLightPositions[i] - vFragPos);
        vec3  H = normalize(V + L);
        float dist        = length(uLightPositions[i] - vFragPos);
        float attenuation = 1.0 / (dist * dist);     // caída física por distancia²
        vec3  radiance    = uLightColors[i] * attenuation;

        // BRDF
        float D = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3  F = FresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3  numerator   = D * G * F;
        float NdotL       = max(dot(N, L), 0.0);
        float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
        vec3  specular    = numerator / denominator;

        // kS = F (energía especular reflejada); kD = energía restante para difuso.
        // Los metales no tienen difuso → se anula con (1 - metallic).
        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // --- Ambiente + composición ---
    vec3 ambient = uAmbientFactor * albedo;
    vec3 color   = ambient + Lo;

    // Tone mapping (Reinhard) y corrección gamma, una sola vez al final.
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
