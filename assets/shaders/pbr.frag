#version 450 core

in vec3 vFragPos;   // posición del fragmento en espacio mundo
in vec2 vUV;
in mat3 vTBN;       // base tangente→mundo (columna 2 = normal)

out vec4 FragColor;

// Constante por frame: view/proj, la matriz light-space, la del skybox, y los
// escalares de sombra e IBL. Lo sube Renderer::BeginScene una vez.
layout(std140, binding = 0) uniform Frame {
    mat4 uView;
    mat4 uProjection;
    mat4 uLightSpaceMatrix;
    mat4 uInvViewProjRot;
    vec4 uViewPos;        // .xyz
    vec4 uShadowParams;   // x=enabled, y=biasMin, z=biasMax, w=normalOffset (m)
    vec4 uIblParams;      // x=hasIbl, y=intensity, z=prefilterMaxLod
};

// Bloque Lights (binding 1), SSBO de locales y visibles, y su matematica.
#include "common/lights.glsl"
#include "common/clusters.glsl"

layout(std140, binding = 3) uniform MaterialParams {
    vec4  uAlbedoTint;    // .rgb
    vec4  uEmissiveTint;  // .rgb
    vec4  uScalars0;      // metallic, roughness, aoStrength, heightScale
    vec4  uScalars1;      // alphaCutoff, emissiveIntensity, normalStrength, _
    uvec4 uMapMask;       // x = bitmask indexado por TextureSlot
    vec4  uUvTransform;   // xy = tiling (repeticiones), zw = offset
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
layout(binding = 8)  uniform sampler2DArray uCascadeMaps;
layout(binding = 9)  uniform samplerCube uIrradianceMap;
layout(binding = 10) uniform samplerCube uPrefilterMap;
layout(binding = 11) uniform sampler2D   uBrdfLUT;

// Bloque de DDGI (binding 5), samplers 12/13, y toda su matematica.
#include "ddgi/common.glsl"

// -- Oclusion ambiental screen-space (binding 6, unidad 14) -------------------
// Bloque propio y no campos nuevos en Frame: extender Frame obliga a tocar los
// cuatro shaders que lo declaran sin que ninguno use el dato.
layout(std140, binding = 6) uniform AoParams {
    vec4 uAoParams;    // enabled, bentNormal, multiBounce, debugView
    // x = la indirecta llega ya resuelta en la unidad 15 (no samplear el volumen)
    // y = el AO esta a resolucion reducida (subirlo con el mismo filtro)
    // z = escala: texels de resolucion completa por texel reducido, por eje
    // w = libre
    //
    // x e y son INDEPENDIENTES: el AO puede estar a resolucion reducida con la
    // indirecta apagada, y ahi hay que subir uno y no el otro.
    vec4 uUpsample;
};
layout(binding = 14) uniform sampler2D uAoTexture;

// -- Cascadas del sol (binding 7, unidad 8) -----------------------------------
// Bloque propio por lo mismo que AoParams: Frame lo declaran nueve shaders y
// solo este necesita las cascadas.
layout(std140, binding = 7) uniform Cascades {
    mat4 uCascadeMatrices[4];
    vec4 uCascadeSplitFar;      // corte lejano de cada cascada (distancia de vista)
    vec4 uCascadeOffsets;       // normal offset de cada cascada, en METROS
    vec4 uCascadeParams;        // x=count (0 = apagado), y=blendRatio, z=debugView
};

// -- Señales resueltas a resolucion reducida (unidades 15 y 16) ---------------
// uIndirect trae la irradiancia indirecta de DDGI ya escalada (rgb) y el fade
// del volumen (a); lo escribe ddgi/indirect.frag a 1/2 por eje. uDepthNormal es
// el prepass del AO A RESOLUCION COMPLETA y es la GUIA del upsample: sin el los
// pesos no tienen contra que comparar.
layout(binding = 15) uniform sampler2D uIndirect;
layout(binding = 16) uniform sampler2D uDepthNormal;

#include "common/bilateral.glsl"

bool UsaIndirecta()  { return uUpsample.x > 0.5; }
bool AoEsReducido()  { return uUpsample.y > 0.5; }
int  EscalaUpsample(){ return int(uUpsample.z + 0.5); }

// La guia solo hace falta si alguno de los dos upsamples esta activo.
bool NecesitaGuia()  { return UsaIndirecta() || AoEsReducido(); }

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

// Multi-rebote de Jimenez: convierte la visibilidad escalar en un factor RGB
// dependiente del albedo. Sin esto, una pared roja ocluida se oscurece hacia
// gris sucio en vez de hacia rojo oscuro -- el aspecto "AO como mugre".
vec3 MultiBounce(float ao, vec3 albedo) {
    vec3 a =  2.0404 * albedo - 0.3324;
    vec3 b = -4.7951 * albedo + 0.6417;
    vec3 c =  2.7552 * albedo + 0.6903;
    return clamp(ao * (ao * (ao * a + b) + c), vec3(ao), vec3(1.0));
}

// Cook-Torrance para una luz. Ld = hacia el centro (difusa), Ls = hacia el
// punto representativo (especular), energia = normalizacion de la esfera. Con
// Ld == Ls y energia 1 es exactamente el Cook-Torrance de un punto.
vec3 CookTorranceBRDF(vec3 N, vec3 V, vec3 Ld, vec3 Ls, float energia,
                      vec3 F0, vec3 albedo, float metallic, float roughness) {
    vec3  H = normalize(V + Ls);
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, Ls, roughness);
    vec3  F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    float NdotLs   = max(dot(N, Ls), 0.0);
    vec3  specular = D * G * F / (4.0 * max(dot(N, V), 0.0) * NdotLs + 0.0001) * NdotLs * energia;

    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    return kD * albedo / PI * max(dot(N, Ld), 0.0) + specular;
}

vec3 EvaluarLuzLocal(GpuLight luz, vec3 N, vec3 V, vec3 F0, vec3 albedo, float metallic, float roughness) {
    vec3  Lvec  = luz.positionRange.xyz - vFragPos;
    float d2    = dot(Lvec, Lvec);
    float rango = luz.positionRange.w;
    if (d2 >= rango * rango) return vec3(0.0);

    float d   = sqrt(d2);
    vec3  l   = Lvec / max(d, 1e-6);
    float att = LightFalloff(d2, rango, luz.colorRadius.w) * SpotAngular(luz, l);
    if (att <= 0.0) return vec3(0.0);

    vec3  ls      = l;
    float energia = 1.0;
    if (luz.colorRadius.w > 0.0) {
        ls      = RepresentativePoint(Lvec, reflect(-V, N), luz.colorRadius.w);
        energia = SphereNormalization(roughness, luz.colorRadius.w, d);
    }
    return CookTorranceBRDF(N, V, l, ls, energia, F0, albedo, metallic, roughness)
         * luz.colorRadius.rgb * att;
}

// Factor de sombra [0=iluminado, 1=en sombra] con PCF 3x3 sobre N cascadas.
// Ng es la normal GEOMETRICA (no la del normal map: el offset de abajo es un
// desplazamiento real en el mundo y no tiene que bailar con la textura) y L
// apunta hacia la luz. Las dos en espacio mundo.

// Profundidad en espacio de VISTA del fragmento, positiva hacia adelante. Se
// compara contra los cortes, que se calcularon en esa misma unidad: usar la
// distancia radial al ojo desalinea la seleccion contra el encuadre y abre una
// banda curva donde la cascada cambia antes de lo que debe.
float ViewDepth() {
    return -(uView * vec4(vFragPos, 1.0)).z;
}

int PickCascade(float viewDepth) {
    int count = int(uCascadeParams.x);
    for (int i = 0; i < count; ++i) {
        if (viewDepth < uCascadeSplitFar[i]) return i;
    }
    return -1;   // mas alla de la ultima: sin sombra
}

float SampleCascade(int i, vec3 Ng, float NdotL, float sinTheta) {
    vec3 muestra = vFragPos + Ng * (uCascadeOffsets[i] * sinTheta);

    vec4 lp   = uCascadeMatrices[i] * vec4(muestra, 1.0);
    vec3 proj = lp.xyz / lp.w;
    proj      = proj * 0.5 + 0.5;
    if (proj.z > 1.0) return 0.0;

    float bias   = max(uShadowParams.z * (1.0 - NdotL), uShadowParams.y);
    float shadow = 0.0;
    vec2  texel  = 1.0 / vec2(textureSize(uCascadeMaps, 0).xy);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float closest = texture(uCascadeMaps,
                                    vec3(proj.xy + vec2(x, y) * texel, float(i))).r;
            shadow += (proj.z - bias > closest) ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

float ShadowFactor(vec3 Ng, vec3 L) {
    // Normal-offset bias: en vez de empujar la profundidad HACIA LA LUZ, corre el
    // punto de muestreo a lo largo de la normal. La diferencia se ve en las
    // aristas entre paredes, donde el oclusor toca al receptor y cualquier bias
    // de profundidad los separa e ilumina una banda a lo largo del rincon
    // (peter-panning).
    //
    // El sin(theta) escala con lo rasante que llega la luz, que es como crece la
    // huella del texel sobre la superficie: cero de frente, maximo al ras.
    float NdotL    = dot(Ng, L);
    float sinTheta = sqrt(clamp(1.0 - NdotL * NdotL, 0.0, 1.0));

    if (uCascadeParams.x < 0.5) return 0.0;

    float profundidad = ViewDepth();
    int   i = PickCascade(profundidad);
    if (i < 0) return 0.0;

    float sombra = SampleCascade(i, Ng, NdotL, sinTheta);

    // La costura entre cascadas es un salto en el ancho del filtro y se lee como
    // una linea recta cruzando el piso. En la fraccion final de cada cascada se
    // samplean las dos y se interpola: 18 taps en vez de 9, solo en la banda.
    int count = int(uCascadeParams.x);
    if (i + 1 < count) {
        float cerca = (i == 0) ? 0.0 : uCascadeSplitFar[i - 1];
        float ancho = uCascadeSplitFar[i] - cerca;
        float banda = ancho * uCascadeParams.y;
        if (banda > 0.0) {
            float t = (profundidad - (uCascadeSplitFar[i] - banda)) / banda;
            if (t > 0.0) {
                sombra = mix(sombra, SampleCascade(i + 1, Ng, NdotL, sinTheta),
                             clamp(t, 0.0, 1.0));
            }
        }
    }
    return sombra;
}

// Color plano por cascada. Es la unica forma de VER donde caen los cortes en vez
// de deducirlo de si la sombra se ve bien.
vec3 CascadeDebugColor() {
    int i = PickCascade(ViewDepth());
    if (i == 0) return vec3(1.0, 0.3, 0.3);
    if (i == 1) return vec3(0.3, 1.0, 0.3);
    if (i == 2) return vec3(0.3, 0.5, 1.0);
    if (i == 3) return vec3(1.0, 1.0, 0.3);
    return vec3(0.5);
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

vec3 HeatRamp(float t) {
    t = clamp(t, 0.0, 1.0);
    vec3 azul  = vec3(0.0, 0.2, 1.0);
    vec3 verde = vec3(0.0, 1.0, 0.2);
    vec3 rojo  = vec3(1.0, 0.1, 0.0);
    return (t < 0.5) ? mix(azul, verde, t * 2.0) : mix(verde, rojo, t * 2.0 - 1.0);
}

void main() {
    // --- Parallax Occlusion Mapping: desplaza las UV antes de muestrear nada ---
    // viewDirT: dirección hacia la cámara en espacio tangente (mundo→tangente
    // vía transpose(TBN), que es la inversa para una base ortonormal).
    vec3 viewDirT = normalize(transpose(vTBN) * (uViewPos.xyz - vFragPos));
    // Tiling del material: la UV de la malla escalada y corrida, comun a los 8
    // mapas. Se aplica ANTES del POM, asi que el POM camina en el espacio ya
    // tileado. heightScale NO se compensa a proposito: medido en unidades de UV,
    // tilear x4 achica la baldosa x4 y el relieve con ella, que es justo lo que
    // conserva la proporcion del ladrillo en vez de dejarlo plano.
    vec2 uvBase = vUV * uUvTransform.xy + uUvTransform.zw;
    vec2 uv = hasMap(SLOT_HEIGHT) ? ParallaxOcclusionMapping(uvBase, viewDirT) : uvBase;

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
    // La geométrica se guarda aparte: ShadowFactor la necesita sin perturbar
    // para correr el punto de muestreo.
    vec3 Ng = normalize(vTBN[2]);
    if (!gl_FrontFacing) { N = -N; Ng = -Ng; }
    vec3 V = normalize(uViewPos.xyz - vFragPos);   // del fragmento hacia la cámara

    // F0: reflectancia base con incidencia normal. Dieléctricos ≈ 0.04;
    // los metales reflejan con su propio color (albedo).
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // --- Luz directa: locales, las del cluster del pixel ---
    // Sin grilla (el pase fallo o esta apagado) se recorren todas las visibles:
    // misma imagen, mas lenta.
    vec3  Lo              = vec3(0.0);
    uint  lucesDelCluster = 0u;
    uint  corteZ          = 0u;
    if (ClustersEnabled()) {
        float profundidad = ViewDepth();
        corteZ = ClusterSlice(profundidad);
        uint base = ClusterIndex(gl_FragCoord.xy, profundidad) * ClusterStride();
        lucesDelCluster = uClusterLights[base];
        for (uint k = 0u; k < lucesDelCluster; ++k) {
            Lo += EvaluarLuzLocal(uLocalLights[uClusterLights[base + 1u + k]],
                                  N, V, F0, albedo, metallic, roughness);
        }
    } else {
        for (uint k = 0u; k < uLightCounts.z; ++k) {
            Lo += EvaluarLuzLocal(uLocalLights[uVisibleLights[k]], N, V, F0, albedo, metallic, roughness);
        }
    }

    // --- Direccionales: el PrimarySun (w = 1) con las cascadas, el resto sin sombra ---
    // 'shadow' vive afuera porque kDdgiViewShadow lo escribe crudo.
    float shadow = 0.0;
    for (uint i = 0u; i < uLightCounts.y; ++i) {
        vec3  Ld = normalize(-uDirDirection[i].xyz);
        float s  = 0.0;
        if (uDirColor[i].w > 0.5) {
            s      = ShadowFactor(Ng, Ld);
            shadow = s;
        }
        Lo += (1.0 - s) * CookTorranceBRDF(N, V, Ld, Ld, 1.0, F0, albedo, metallic, roughness)
            * uDirColor[i].rgb;
    }

    // --- Luz indirecta: IBL difuso + especular (split-sum) ---
    // El AO solo modula la indirecta, nunca la directa. uAOStrength interpola entre
    // "sin oclusión" (1.0) y el valor del mapa.
    float ao    = hasMap(SLOT_AO) ? mix(1.0, texture(uAOMap, uv).r, uScalars0.z) : 1.0;

    // -- La guia del upsample -------------------------------------------------
    // Depth y normal de ESTE pixel en espacio de VISTA, que es donde vive el
    // prepass. Se calculan una sola vez y los usan los dos upsamples (el del AO
    // y el de la indirecta).
    //
    // La normal es la GEOMETRICA y no la del normal map, a proposito: el prepass
    // escribe la geometrica, y comparar contra una normal perturbada por textura
    // rechazaria taps de la misma superficie plana en cuanto el mapa tenga algo
    // de relieve.
    float zGuia = 0.0;
    vec3  nGuia = vec3(0.0, 0.0, 1.0);
    if (NecesitaGuia()) {
        zGuia = -(uView * vec4(vFragPos, 1.0)).z;   // positivo: la convencion del prepass
        nGuia = normalize(mat3(uView) * Ng);
    }
    ivec2 pixel = ivec2(gl_FragCoord.xy);

    // AO screen-space: el contacto sub-metrico que la grilla de probes no ve.
    // A resolucion completa es un texelFetch directo; a resolucion reducida hay
    // que subirlo con el mismo filtro bilateral que la indirecta, o el bent
    // normal cruza siluetas y la visibilidad del fondo se derrama sobre el borde
    // de los objetos.
    vec4 aoMuestra = AoEsReducido()
                   ? BilateralUpsample(uAoTexture, uDepthNormal, pixel,
                                       zGuia, nGuia, EscalaUpsample())
                   : texelFetch(uAoTexture, pixel, 0);
    float ssVis     = (uAoParams.x > 0.5) ? clamp(aoMuestra.w, 0.0, 1.0) : 1.0;

    // El bent normal apunta hacia donde el hemisferio esta ABIERTO. Es lo que
    // hace que el piso pegado a la pared roja consulte la irradiancia hacia el
    // interior de la sala y no hacia arriba.
    vec3 bentCrudo = aoMuestra.xyz;
    vec3 bentN = (uAoParams.y > 0.5 && dot(bentCrudo, bentCrudo) > 1e-8)
               ? normalize(bentCrudo)
               : N;

    float NdotV = max(dot(N, V), 0.0);

    // F con corrección por rugosidad: acá SÍ se usa el resultado, no solo para kD.
    vec3 F  = FresnelSchlickRoughness(NdotV, F0, roughness);
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);   // los metales no tienen difuso

    // --- Luz indirecta ---
    // La difusa y la especular se separan porque solo la difusa la reemplaza
    // DDGI. Antes las dos vivian dentro de un solo if de IBL; ahora la difusa
    // tiene que poder venir de DDGI aunque no haya entorno IBL cargado.

    // Difusa: DDGI reemplaza al IBL adentro del volumen. NO se suma -- sumar
    // contaria la misma luz dos veces y lavaria todo. El fade sobre la celda del
    // borde evita la costura dura donde el volumen termina y queda solo el IBL.
    vec3 iblIrr = (uIblParams.x > 0.5)
                ? texture(uIrradianceMap, bentN).rgb * uIblParams.y
                : vec3(0.0);

    // ddgiFade y ddgiIrr se hoistean fuera del if porque los modos de debug del
    // final los leen. Recalcularlos alla abajo es la forma exacta de que el
    // debug y la imagen se desincronicen y el instrumento mienta.
    float ddgiFade = 0.0;
    vec3  ddgiIrr  = vec3(0.0);

    vec3 indirectDiffuse = iblIrr;
    if (UsaIndirecta()) {
        // -- Camino rapido: la indirecta ya esta resuelta ---------------------
        // ddgi/indirect.frag la calculo a 1/2 por eje con la MISMA matematica
        // (mismo bias, mismo Chebyshev, misma intensidad) y dejo el fade en el
        // alfa. Aca solo se sube de resolucion. Son 4 taps de la indirecta y 4
        // de la guia -- ocho fetches locales y coherentes en cache -- contra los
        // ~16 gathers dispersos del sampleo del volumen.
        vec4 muestra = BilateralUpsample(uIndirect, uDepthNormal, pixel,
                                         zGuia, nGuia, EscalaUpsample());
        ddgiIrr  = muestra.rgb;
        ddgiFade = muestra.a;
        if (ddgiFade > 0.0) indirectDiffuse = mix(iblIrr, ddgiIrr, ddgiFade);
    } else if (DdgiEnabled()) {
        // -- Camino inline: samplear el volumen por pixel ----------------------
        // Es el camino de antes de que existiera IndirectPass, y sigue vivo
        // porque ese pase depende del prepass del AO: con el AO apagado, o si
        // fallo la carga de su shader, esto es lo unico que hay. Cuesta lo que
        // cuesta -- ver el comentario de IndirectPass.h.
        ddgiFade = DdgiVolumeFade(vFragPos);

        // Con fade 0 el sampleo se saltea: son 8 taps de irradiancia y 8 de
        // distancia que no van a ningun lado. La excepcion es el modo que existe
        // justamente para ver lo que el fade descarta.
        if (ddgiFade > 0.0 || DdgiDebugView() == kDdgiViewDdgiNoFade) {
            // Ablation test: irradiancia constante, MISMO camino aguas abajo.
            // Lo unico que desaparece son los gathers del volumen. Ver
            // DdgiSettings::ablateSample.
            ddgiIrr = (DdgiAblate() ? vec3(DdgiAblateIrradiance())
                                    : SampleDdgiIrradiance(vFragPos, N, bentN, V))
                    * uDdgiParams0.y;
        }
        if (ddgiFade > 0.0) indirectDiffuse = mix(iblIrr, ddgiIrr, ddgiFade);
    }

    // Especular: sigue siendo IBL sin cambios. Eso lo completa SSR (ciclo 3).
    vec3 specularIBL = vec3(0.0);
    if (uIblParams.x > 0.5) {
        vec3 R           = reflect(-V, N);
        vec3 prefiltered = textureLod(uPrefilterMap, R, roughness * uIblParams.z).rgb;
        vec2 ab          = texture(uBrdfLUT, vec2(NdotV, roughness)).rg;
        specularIBL = prefiltered * (F * ab.x + ab.y) * uIblParams.y;
    }

    // Separado del especular para que el modo de debug pueda mostrar EXACTAMENTE
    // lo que la indirecta difusa le suma al pixel, sin reconstruirlo aparte.
    // Algebraicamente es la misma suma de antes.
    // El mapa de AO del material y el GTAO son oclusion a escalas distintas
    // (micro-detalle horneado vs. contacto de escena): se componen multiplicando.
    float aoTotal = ao * ssVis;
    vec3  aoRgb   = (uAoParams.z > 0.5) ? MultiBounce(aoTotal, albedo) : vec3(aoTotal);

    vec3 indirectApplied = kD * indirectDiffuse * albedo * aoRgb;
    vec3 ambient         = indirectApplied + specularIBL * aoRgb;

    // --- Emision propia: no la toca el AO, ni la sombra, ni la intensidad de IBL ---
    // Sale en HDR lineal, así que florece con bloom recién cuando la intensidad
    // supera el threshold del brightpass.
    vec3 emissive = (hasMap(SLOT_EMISSIVE) ? texture(uEmissiveMap, uv).rgb : vec3(1.0))
                  * uEmissiveTint.rgb * uScalars1.y;

    vec3 color = ambient + Lo + emissive;

    // --- Modos de debug de vista ---
    // Escriben UN termino en lugar de la suma. Salen por el mismo camino HDR, o
    // sea que bloom y ACES los tocan igual: son CUALITATIVOS -- responden "este
    // termino aporta algo o aporta cero", no dan un numero. Con el threshold de
    // bloom en 1.0 y estos valores casi siempre por debajo, el unico que llega a
    // florecer es el de luz directa.
    switch (DdgiDebugView()) {
        case kDdgiViewIndirect:        color = indirectDiffuse; break;
        case kDdgiViewIndirectApplied: color = indirectApplied; break;
        case kDdgiViewDirect:          color = Lo;              break;
        case kDdgiViewDdgiNoFade:      color = ddgiIrr;         break;
        case kDdgiViewFade:            color = vec3(ddgiFade);  break;
        case kDdgiViewAlbedo:          color = albedo;          break;
        case kDdgiViewNormal:          color = N * 0.5 + 0.5;   break;
        // El termino de sombra crudo, sin albedo ni especular encima: blanco =
        // el sol llega, negro = tapado. Es el unico modo que sirve para medir
        // una fuga, porque en la imagen final una fuga sobre pared oscura se ve
        // BLANCA (el lobulo especular no se multiplica por el albedo) y no hay
        // forma de saber cuanta sombra falta.
        case kDdgiViewShadow:          color = vec3(1.0 - shadow); break;
        default: break;   // kDdgiViewOff: la imagen final
    }

    // Las vistas del AO corren DESPUES de las de DDGI: si las dos estan activas,
    // gana AO. Los modos 3 y 4 muestran lo que gtao.frag volco del prepass.
    int aoVista = int(uAoParams.w + 0.5);
    if      (aoVista == 1) color = vec3(ssVis);                    // visibilidad
    else if (aoVista == 2) color = aoMuestra.xyz * 0.5 + 0.5;      // bent normal (world)
    else if (aoVista == 3) color = aoMuestra.xyz * 0.5 + 0.5;      // normal del prepass (view)
    else if (aoVista == 4) color = aoMuestra.xyz;                  // viewZ, una banda por metro

    // Vistas de clusters: despues de las de DDGI/AO, asi que ganan si hay dos.
    int vistaClusters = int(uClusterScreen.y + 0.5);
    if (ClustersEnabled() && vistaClusters == 1) {
        vec3 calor = (lucesDelCluster == 0u)            ? vec3(0.0)
                   : (lucesDelCluster >= uClusterDims.w) ? vec3(1.0, 0.0, 1.0)   // saturado
                   : HeatRamp(float(lucesDelCluster) / float(uClusterDims.w));
        color = calor * 0.6 + 0.4 * color / (1.0 + color);
    } else if (ClustersEnabled() && vistaClusters == 2) {
        const vec3 kBandas[4] = vec3[4](vec3(0.9, 0.3, 0.3), vec3(0.3, 0.9, 0.3),
                                        vec3(0.3, 0.4, 0.9), vec3(0.9, 0.9, 0.3));
        color = kBandas[corteZ % 4u] * 0.8 + 0.2 * color / (1.0 + color);
    }

    // Radiancia lineal HDR sin tonemapear: el tone mapping + gamma ahora ocurren
    // una sola vez en el present pass (assets/shaders/tonemap.frag), Ciclo 1 HDR.
    if (uCascadeParams.z > 0.5) {
        FragColor = vec4(CascadeDebugColor(), 1.0);
        return;
    }

    FragColor = vec4(color, 1.0);
}
