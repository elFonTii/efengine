#version 450 core
// Sombreado de la captura de probes. Difuso PURO: sin Cook-Torrance, sin GGX,
// sin normal map, sin POM. No es una copia degradada de pbr.frag -- es un
// modelo de sombreado distinto, el unico que la irradiancia difusa usa. A
// 16x16 por cara, el detalle sub-texel se promedia igual.
//
// Escribe radiancia en rgb y DISTANCIA en el alfa: de ahi sale el test de
// Chebyshev que mata el light leaking.
//
// Reusa pbr.vert verbatim, asi que recibe las mismas varyings.

in vec3 vFragPos;
in vec2 vUV;
in mat3 vTBN;

out vec4 FragColor;

#define MAX_LIGHTS 4

layout(std140, binding = 0) uniform Frame {
    mat4 uView;
    mat4 uProjection;
    mat4 uLightSpaceMatrix;
    mat4 uInvViewProjRot;
    vec4 uViewPos;        // .xyz = CENTRO DEL PROBE en este pase, no la camara
    vec4 uShadowParams;   // x=enabled, y=biasMin, z=biasMax, w=normalOffset (m)
    vec4 uIblParams;
};

layout(std140, binding = 1) uniform Lights {
    vec4  uLightPositions[MAX_LIGHTS];
    vec4  uLightColors[MAX_LIGHTS];
    vec4  uLightDir;
    vec4  uDirLightColor;
    ivec4 uLightCounts;
};

layout(std140, binding = 3) uniform MaterialParams {
    vec4  uAlbedoTint;
    vec4  uEmissiveTint;
    vec4  uScalars0;      // metallic, roughness, aoStrength, heightScale
    vec4  uScalars1;      // alphaCutoff, emissiveIntensity, normalStrength, _
    uvec4 uMapMask;
    vec4  uUvTransform;   // xy = tiling, zw = offset
};

layout(binding = 0) uniform sampler2D uAlbedoMap;
layout(binding = 6) uniform sampler2D uOpacityMap;
layout(binding = 7) uniform sampler2D uEmissiveMap;
layout(binding = 8) uniform sampler2D uShadowMap;

#include "ddgi/common.glsl"

const uint SLOT_ALBEDO   = 0u;
const uint SLOT_OPACITY  = 6u;
const uint SLOT_EMISSIVE = 7u;

bool hasMap(uint slot) { return (uMapMask.x & (1u << slot)) != 0u; }

// Misma logica que ShadowFactor de pbr.frag: PCF 3x3 con normal-offset bias.
// Tiene que seguir siendo la misma: si el probe ve una banda de luz en el
// rincon que la vista directa no ve, esa banda se hornea en la irradiancia y
// sale por todos lados. N aca ya es la geometrica (main la calcula asi).
float ShadowFactor(vec3 N, vec3 L) {
    float NdotL    = dot(N, L);
    float sinTheta = sqrt(clamp(1.0 - NdotL * NdotL, 0.0, 1.0));
    vec3  muestra  = vFragPos + N * (uShadowParams.w * sinTheta);

    vec4 lp   = uLightSpaceMatrix * vec4(muestra, 1.0);
    vec3 proj = lp.xyz / lp.w;
    proj      = proj * 0.5 + 0.5;
    if (proj.z > 1.0) return 0.0;

    float bias   = max(uShadowParams.z * (1.0 - NdotL), uShadowParams.y);
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

void main() {
    // El mismo tiling que pbr.frag, y no es opcional: si la captura samplea el
    // albedo sin tilear, el color que se hornea en la irradiancia no es el de
    // ninguna superficie de la escena y el rebote tiñe todo de un color inventado.
    vec2 uv = vUV * uUvTransform.xy + uUvTransform.zw;

    float alpha = hasMap(SLOT_OPACITY) ? texture(uOpacityMap, uv).r : 1.0;
    if (alpha < uScalars1.x) discard;

    vec3 albedo = hasMap(SLOT_ALBEDO)
                ? texture(uAlbedoMap, uv).rgb * uAlbedoTint.rgb
                : uAlbedoTint.rgb;

    // Normal geometrica: el normal map es detalle de alta frecuencia que la
    // irradiancia difusa promedia igual.
    vec3 N = normalize(vTBN[2]);
    if (!gl_FrontFacing) N = -N;

    // uViewPos es el centro del probe en este pase.
    float dist = length(uViewPos.xyz - vFragPos);

    // --- Luz directa, Lambert puro ---
    vec3 direct = vec3(0.0);
    for (int i = 0; i < uLightCounts.x; ++i) {
        vec3  d   = uLightPositions[i].xyz - vFragPos;
        float dd  = max(dot(d, d), 0.0001);
        vec3  L   = d / sqrt(dd);
        direct += albedo / kDdgiPI * max(dot(N, L), 0.0) * uLightColors[i].rgb / dd;
    }
    {
        vec3  Ld     = normalize(-uLightDir.xyz);
        float shadow = (uShadowParams.x > 0.5) ? ShadowFactor(N, Ld) : 0.0;
        direct += (1.0 - shadow) * albedo / kDdgiPI * max(dot(N, Ld), 0.0)
                * uDirLightColor.rgb;
    }

    // --- El rebote: irradiancia del atlas del FRAME ANTERIOR ---
    // De aca salen los rebotes infinitos, gratis: la captura corre antes del
    // blend de este frame, asi que lee lo que el blend anterior dejo. Cada
    // frame apila un rebote sobre el resultado del anterior y la serie converge
    // sola a la solucion multi-rebote, sin trazar un rayo mas.
    //
    // albedo pelado y sin /PI: kD = 1 porque es difuso puro, y el atlas guarda
    // radiancia media (no irradiancia), asi que E = PI * L y el /PI del BRDF se
    // cancela. Queda dimensionalmente igual que `direct` de arriba.
    //
    // NO se multiplica por la intensidad artistica (params0.y) A PROPOSITO: el
    // lazo esta realimentado, asi que la ganancia por vuelta seria
    // albedo * intensidad. Con intensidad > 1 y un albedo alto eso pasa de 1 y
    // la energia diverge frame a frame hasta saturar. Sin ella la ganancia es
    // albedo < 1 y converge siempre. La intensidad se aplica una sola vez,
    // cuando pbr.frag samplea.
    vec3 bounce = vec3(0.0);
    if (DdgiEnabled()) {
        // "Hacia el probe" hace de V: en este pase el observador es el centro
        // del probe y no la camara, y viewBias empuja el punto de muestreo hacia
        // el. El guard es por el fragmento que cae justo sobre el centro, donde
        // normalize(0) daria NaN y lo propagaria a todo el atlas via el blend.
        vec3  hacia = uViewPos.xyz - vFragPos;
        float dProb = length(hacia);
        hacia       = (dProb > 1e-5) ? hacia / dProb : N;

        bounce = albedo * SampleDdgiIrradiance(vFragPos, N, hacia)
               * DdgiVolumeFade(vFragPos);
    }

    vec3 emissive = (hasMap(SLOT_EMISSIVE) ? texture(uEmissiveMap, uv).rgb : vec3(1.0))
                  * uEmissiveTint.rgb * uScalars1.y;

    FragColor = vec4(direct + bounce + emissive, dist);
}
