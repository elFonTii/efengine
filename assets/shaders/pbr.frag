#version 450 core

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

// --- Luz direccional (sol) ---
uniform vec3 uLightDir;       // dirección en la que viaja la luz
uniform vec3 uDirLightColor;  // color * intensidad (sin atenuación por distancia)

// --- Sombra direccional (shadow map + PCF) ---
uniform mat4      uLightSpaceMatrix;
uniform sampler2D uShadowMap;
uniform int       uShadowEnabled;
uniform float     uShadowBiasMin;
uniform float     uShadowBiasMax;

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

// --- Normal ---
uniform sampler2D uNormalMap;
uniform int       uHasNormalMap;

// --- Ambient Occlusion ---
uniform sampler2D uAOMap;
uniform int       uHasAOMap;
uniform float     uAOStrength;   // 0 = sin AO, 1 = AO pleno

// --- IBL difuso (mapa de irradiancia precomputado) ---
uniform samplerCube uIrradianceMap;

// --- Height / Displacement (Parallax Occlusion Mapping) ---
uniform sampler2D uHeightMap;
uniform int       uHasHeightMap;
uniform float     uHeightScale;  // profundidad del relieve (p. ej. 0.05)

uniform sampler2D uOpacityMap;
uniform int       uHasOpacityMap;
uniform float     uAlphaCutoff;

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

// Fresnel-Schlick con corrección por rugosidad, para la reflexión ambiente:
// en superficies rugosas la reflectancia en rasante no debe dispararse a 1.
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0)
              * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Cook-Torrance para una luz: devuelve (kD*albedo/PI + specular) * NdotL.
// La radiancia (color/atenuación) se multiplica fuera.
vec3 CookTorranceBRDF(vec3 N, vec3 V, vec3 L, vec3 F0, vec3 albedo, float metallic, float roughness) {
    vec3  H = normalize(V + L);
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3  F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3  numerator   = D * G * F;
    float NdotL       = max(dot(N, L), 0.0);
    float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
    vec3  specular    = numerator / denominator;

    // kS = F (especular); kD = resto para difuso. Los metales no tienen difuso.
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    return (kD * albedo / PI + specular) * NdotL;
}

// Factor de sombra [0=iluminado, 1=en sombra] con PCF 3×3. N,L en espacio mundo.
float ShadowFactor(vec3 N, vec3 L) {
    vec4 lp   = uLightSpaceMatrix * vec4(vFragPos, 1.0);
    vec3 proj = lp.xyz / lp.w;          // divide perspectiva (ortho: w=1)
    proj      = proj * 0.5 + 0.5;       // [-1,1] → [0,1]
    if (proj.z > 1.0) return 0.0;       // más allá del far plane → sin sombra

    // Bias slope-scaled: más grande en rasante para matar el acné.
    float bias   = max(uShadowBiasMax * (1.0 - dot(N, L)), uShadowBiasMin);
    float shadow = 0.0;
    vec2  texel  = 1.0 / vec2(textureSize(uShadowMap, 0));
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float closest = texture(uShadowMap, proj.xy + vec2(x, y) * texel).r;
            shadow += (proj.z - bias > closest) ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

// Parallax Occlusion Mapping: desplaza la UV según el ángulo de visión para
// simular profundidad real. viewDirT está en espacio tangente. El mapa se
// interpreta como altura (blanco = alto); usamos profundidad = 1 - altura.
vec2 ParallaxOcclusionMapping(vec2 uv, vec3 viewDirT) {
    // Más capas cuando miramos en rasante (donde el efecto es más notorio).
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, max(viewDirT.z, 0.0));

    float layerDepth        = 1.0 / numLayers;
    float currentLayerDepth = 0.0;

    // Desplazamiento de UV por capa (offset limiting: sin dividir por z).
    vec2 deltaUV = (viewDirT.xy * uHeightScale) / numLayers;

    vec2  currentUV    = uv;
    float currentDepth = 1.0 - texture(uHeightMap, currentUV).r;

    // Avanzamos capa por capa hasta cruzar la superficie del relieve.
    while (currentLayerDepth < currentDepth) {
        currentUV          -= deltaUV;
        currentDepth        = 1.0 - texture(uHeightMap, currentUV).r;
        currentLayerDepth  += layerDepth;
    }

    // Interpolamos entre la capa actual y la anterior para suavizar el escalón.
    vec2  prevUV      = currentUV + deltaUV;
    float afterDepth  = currentDepth - currentLayerDepth;
    float beforeDepth = (1.0 - texture(uHeightMap, prevUV).r) - currentLayerDepth + layerDepth;
    float weight      = afterDepth / (afterDepth - beforeDepth);

    return mix(currentUV, prevUV, weight);
}

void main() {
    // --- Parallax Occlusion Mapping: desplaza las UV antes de muestrear nada ---
    // viewDirT: dirección hacia la cámara en espacio tangente (mundo→tangente
    // vía transpose(TBN), que es la inversa para una base ortonormal).
    vec3 viewDirT = normalize(transpose(vTBN) * (uViewPos - vFragPos));
    vec2 uv = (uHasHeightMap == 1) ? ParallaxOcclusionMapping(vUV, viewDirT) : vUV;

    // No se recorta la UV desplazada: el POM solo se usa sobre superficies con
    // material tileable (UV continua + GL_REPEAT), donde el offset envuelve sin
    // costura. Un discard a [0,1] solo tendria sentido en un quad unico mapeado
    // exactamente a [0,1], y romperia tanto el tiling como las mallas atlaseadas.

    float alpha = (uHasOpacityMap == 1) ? texture(uOpacityMap, uv).r : 1.0;
    if (alpha < uAlphaCutoff) discard;

    // --- Propiedades del material (textura si existe, escalar si no) ---
    vec3 albedo = (uHasAlbedoMap == 1)
                ? texture(uAlbedoMap, uv).rgb * uAlbedoTint
                : uAlbedoTint;

    float metallic  = (uHasMetallicMap == 1)  ? texture(uMetallicMap, uv).r  : uMetallic;
    float roughness = (uHasRoughnessMap == 1) ? texture(uRoughnessMap, uv).r : uRoughness;

    // Normal: del normal map (tangente→mundo vía TBN) o la geométrica como fallback.
    vec3 N;
    if (uHasNormalMap == 1) {
        vec3 n = texture(uNormalMap, uv).rgb * 2.0 - 1.0;  // [0,1] → [-1,1]
        N = normalize(vTBN * n);
    } else {
        N = normalize(vTBN[2]);
    }
    if (!gl_FrontFacing) N = -N;
    vec3 V = normalize(uViewPos - vFragPos);   // del fragmento hacia la cámara

    // F0: reflectancia base con incidencia normal. Dieléctricos ≈ 0.04;
    // los metales reflejan con su propio color (albedo).
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // --- Luz directa: puntuales (Cook-Torrance vía helper) ---
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < uLightCount; ++i) {
        vec3  L           = normalize(uLightPositions[i] - vFragPos);
        float dist        = length(uLightPositions[i] - vFragPos);
        float attenuation = 1.0 / (dist * dist);     // caída física por distancia²
        vec3  radiance    = uLightColors[i] * attenuation;

        Lo += CookTorranceBRDF(N, V, L, F0, albedo, metallic, roughness) * radiance;
    }

    // --- Luz direccional (sol): sin atenuación, con sombra PCF ---
    {
        vec3  Ld     = normalize(-uLightDir);
        float shadow = (uShadowEnabled == 1) ? ShadowFactor(N, Ld) : 0.0;
        Lo += (1.0 - shadow) * CookTorranceBRDF(N, V, Ld, F0, albedo, metallic, roughness) * uDirLightColor;
    }

    // --- Ambiente + composición ---
    // El AO solo modula la luz indirecta (ambiente), no la directa.
    // uAOStrength interpola entre "sin oclusión" (1.0) y el valor del mapa.
    // --- Ambiente por IBL difuso ---
    // El irradiance map da la luz difusa entrante para la normal N. kD descuenta
    // la fracción especular (Fresnel) para no duplicar energía; los metales no
    // tienen difuso. El AO solo modula esta luz indirecta.
    float ao  = (uHasAOMap == 1) ? mix(1.0, texture(uAOMap, uv).r, uAOStrength) : 1.0;
    vec3  F   = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3  kD  = (vec3(1.0) - F) * (1.0 - metallic);
    vec3  irr = texture(uIrradianceMap, N).rgb;
    vec3  ambient = kD * irr * albedo * ao;
    vec3  color   = ambient + Lo;

    // Radiancia lineal HDR sin tonemapear: el tone mapping + gamma ahora ocurren
    // una sola vez en el present pass (assets/shaders/tonemap.frag), Ciclo 1 HDR.
    FragColor = vec4(color, 1.0);
}
