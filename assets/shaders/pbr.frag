#version 450 core

in vec3 vFragPos;   // posición del fragmento en espacio mundo
in vec2 vUV;
in mat3 vTBN;       // base tangente→mundo (columna 2 = normal)

out vec4 FragColor;

#define MAX_LIGHTS 4

// Constante por frame: view/proj, la matriz light-space, la del skybox, y los
// escalares de sombra e IBL. Lo sube Renderer::BeginScene una vez.
layout(std140, binding = 0) uniform Frame {
    mat4 uView;
    mat4 uProjection;
    mat4 uLightSpaceMatrix;
    mat4 uInvViewProjRot;
    vec4 uViewPos;        // .xyz
    vec4 uShadowParams;   // x=enabled, y=biasMin, z=biasMax
    vec4 uIblParams;      // x=hasIbl, y=intensity, z=prefilterMaxLod
};

layout(std140, binding = 1) uniform Lights {
    vec4  uLightPositions[MAX_LIGHTS];  // .xyz
    vec4  uLightColors[MAX_LIGHTS];     // .rgb
    vec4  uLightDir;                    // .xyz — direccion en la que VIAJA la luz
    vec4  uDirLightColor;               // .rgb
    ivec4 uLightCounts;                 // x = cantidad de puntuales activas
};

layout(std140, binding = 3) uniform MaterialParams {
    vec4  uAlbedoTint;    // .rgb
    vec4  uEmissiveTint;  // .rgb
    vec4  uScalars0;      // metallic, roughness, aoStrength, heightScale
    vec4  uScalars1;      // alphaCutoff, emissiveIntensity, normalStrength, _
    uvec4 uMapMask;       // x = bitmask indexado por TextureSlot
};

// Unidades 0-7: los mapas de material, en el orden de renderer::TextureSlot.
// Ese orden es parte del formato .efe y no se toca.
layout(binding = 0) uniform sampler2D uAlbedoMap;
layout(binding = 1) uniform sampler2D uNormalMap;
layout(binding = 2) uniform sampler2D uAOMap;
layout(binding = 3) uniform sampler2D uRoughnessMap;
layout(binding = 4) uniform sampler2D uMetallicMap;
layout(binding = 5) uniform sampler2D uHeightMap;
layout(binding = 6) uniform sampler2D uOpacityMap;
layout(binding = 7) uniform sampler2D uEmissiveMap;

// De 8 en adelante: los mapas de frame.
layout(binding = 8)  uniform sampler2D   uShadowMap;
layout(binding = 9)  uniform samplerCube uIrradianceMap;
layout(binding = 10) uniform samplerCube uPrefilterMap;
layout(binding = 11) uniform sampler2D   uBrdfLUT;

// Espeja renderer::TextureSlot: un bit por slot en uMapMask.x.
const uint SLOT_ALBEDO    = 0u;
const uint SLOT_NORMAL    = 1u;
const uint SLOT_AO        = 2u;
const uint SLOT_ROUGHNESS = 3u;
const uint SLOT_METALLIC  = 4u;
const uint SLOT_HEIGHT    = 5u;
const uint SLOT_OPACITY   = 6u;
const uint SLOT_EMISSIVE  = 7u;

bool hasMap(uint slot) { return (uMapMask.x & (1u << slot)) != 0u; }

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
    float bias   = max(uShadowParams.z * (1.0 - dot(N, L)), uShadowParams.y);
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
    vec2 deltaUV = (viewDirT.xy * uScalars0.w) / numLayers;

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
    vec3 viewDirT = normalize(transpose(vTBN) * (uViewPos.xyz - vFragPos));
    vec2 uv = hasMap(SLOT_HEIGHT) ? ParallaxOcclusionMapping(vUV, viewDirT) : vUV;

    // No se recorta la UV desplazada: el POM solo se usa sobre superficies con
    // material tileable (UV continua + GL_REPEAT), donde el offset envuelve sin
    // costura. Un discard a [0,1] solo tendria sentido en un quad unico mapeado
    // exactamente a [0,1], y romperia tanto el tiling como las mallas atlaseadas.

    float alpha = hasMap(SLOT_OPACITY) ? texture(uOpacityMap, uv).r : 1.0;
    if (alpha < uScalars1.x) discard;

    // --- Propiedades del material (textura si existe, escalar si no) ---
    vec3 albedo = hasMap(SLOT_ALBEDO)
                ? texture(uAlbedoMap, uv).rgb * uAlbedoTint.rgb
                : uAlbedoTint.rgb;

    float metallic  = hasMap(SLOT_METALLIC)  ? texture(uMetallicMap, uv).r  : uScalars0.x;
    float roughness = hasMap(SLOT_ROUGHNESS) ? texture(uRoughnessMap, uv).r : uScalars0.y;

    // Normal: del normal map (tangente→mundo vía TBN) o la geométrica como fallback.
    vec3 N;
    if (hasMap(SLOT_NORMAL)) {
        vec3 n = texture(uNormalMap, uv).rgb * 2.0 - 1.0;  // [0,1] → [-1,1]
        // Escalar solo XY inclina más o menos la normal sin sacarla del hemisferio;
        // la renormalización de abajo recompone Z.
        n.xy *= uScalars1.z;
        N = normalize(vTBN * n);
    } else {
        N = normalize(vTBN[2]);
    }
    if (!gl_FrontFacing) N = -N;
    vec3 V = normalize(uViewPos.xyz - vFragPos);   // del fragmento hacia la cámara

    // F0: reflectancia base con incidencia normal. Dieléctricos ≈ 0.04;
    // los metales reflejan con su propio color (albedo).
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // --- Luz directa: puntuales (Cook-Torrance vía helper) ---
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < uLightCounts.x; ++i) {
        vec3  L           = normalize(uLightPositions[i].xyz - vFragPos);
        float dist        = length(uLightPositions[i].xyz - vFragPos);
        float attenuation = 1.0 / (dist * dist);     // caída física por distancia²
        vec3  radiance    = uLightColors[i].rgb * attenuation;

        Lo += CookTorranceBRDF(N, V, L, F0, albedo, metallic, roughness) * radiance;
    }

    // --- Luz direccional (sol): sin atenuación, con sombra PCF ---
    {
        vec3  Ld     = normalize(-uLightDir.xyz);
        float shadow = (uShadowParams.x > 0.5) ? ShadowFactor(N, Ld) : 0.0;
        Lo += (1.0 - shadow) * CookTorranceBRDF(N, V, Ld, F0, albedo, metallic, roughness) * uDirLightColor.rgb;
    }

    // --- Luz indirecta: IBL difuso + especular (split-sum) ---
    // El AO solo modula la indirecta, nunca la directa. uAOStrength interpola entre
    // "sin oclusión" (1.0) y el valor del mapa.
    float ao    = hasMap(SLOT_AO) ? mix(1.0, texture(uAOMap, uv).r, uScalars0.z) : 1.0;
    float NdotV = max(dot(N, V), 0.0);

    // F con corrección por rugosidad: acá SÍ se usa el resultado, no solo para kD.
    vec3 F  = FresnelSchlickRoughness(NdotV, F0, roughness);
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);   // los metales no tienen difuso

    vec3 ambient = vec3(0.0);
    if (uIblParams.x > 0.5) {
        vec3 diffuseIBL = kD * texture(uIrradianceMap, N).rgb * albedo;

        // El vector reflejado indexa el prefiltrado; la rugosidad elige el mip.
        vec3 R          = reflect(-V, N);
        vec3 prefiltered = textureLod(uPrefilterMap, R, roughness * uIblParams.z).rgb;
        vec2 ab          = texture(uBrdfLUT, vec2(NdotV, roughness)).rg;
        vec3 specularIBL = prefiltered * (F * ab.x + ab.y);

        ambient = (diffuseIBL + specularIBL) * ao * uIblParams.y;
    }

    // --- Emision propia: no la toca el AO, ni la sombra, ni la intensidad de IBL ---
    // Sale en HDR lineal, así que florece con bloom recién cuando la intensidad
    // supera el threshold del brightpass.
    vec3 emissive = (hasMap(SLOT_EMISSIVE) ? texture(uEmissiveMap, uv).rgb : vec3(1.0))
                  * uEmissiveTint.rgb * uScalars1.y;

    vec3 color = ambient + Lo + emissive;

    // Radiancia lineal HDR sin tonemapear: el tone mapping + gamma ahora ocurren
    // una sola vez en el present pass (assets/shaders/tonemap.frag), Ciclo 1 HDR.
    FragColor = vec4(color, 1.0);
}
