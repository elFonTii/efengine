// assets/shaders/ddgi/common.glsl
// LA matematica de DDGI del repo. La incluyen pbr.frag, ddgi/capture.frag,
// los dos blends y la debug viz: cinco consumidores, una fuente. Duplicar
// Chebyshev o el octaedral era el bug que se arregla en una copia y no en la
// otra, sobre la parte mas dificil del sistema.
//
// Requiere que el shader que incluye esto declare el bloque de binding 5 via
// el include de abajo, y -- si va a llamar SampleDdgiIrradiance -- que las
// unidades 12 y 13 tengan los atlas bindeados.

#include "common/cubeface.glsl"

// -- Bloque de constantes de DDGI (binding 5) --------------------------------
layout(std140, binding = 5) uniform Ddgi {
    vec4  uDdgiOrigin;      // .xyz
    vec4  uDdgiSpacing;     // .xyz
    ivec4 uDdgiCounts;      // .xyz = probes por eje, .w = total
    ivec4 uDdgiAtlas;       // x=cols, y=rows, z=irrTile, w=distTile
    ivec4 uDdgiRange;       // x=firstProbe, y=count, z=faceSize, w=probesPerFrame
    vec4  uDdgiParams0;     // hysteresis, intensity, normalBias, viewBias
    vec4  uDdgiParams1;     // enabled, chebyshevSharpness, _, _
};

layout(binding = 12) uniform sampler2D uDdgiIrradiance;
layout(binding = 13) uniform sampler2D uDdgiDistance;

const int   kDdgiBorder = 1;
const float kDdgiPI     = 3.14159265359;

bool DdgiEnabled() { return uDdgiParams1.x > 0.5; }

// -- Mapeo octaedrico --------------------------------------------------------
// [Cigolle et al. 2014]. Proyecta la esfera al cuadrado [-1,1]^2 con distorsion
// de area baja y, lo que importa aca, envolviendo sobre si mismo: por eso el
// borde de un tile se puede llenar espejando texels INTERIORES del mismo tile.

vec2 OctEncode(vec3 dir) {
    float l1 = abs(dir.x) + abs(dir.y) + abs(dir.z);
    vec2  p  = dir.xy * (1.0 / l1);
    if (dir.z < 0.0) {
        // Pliega el hemisferio inferior hacia afuera del rombo.
        p = (1.0 - abs(p.yx)) * vec2(p.x >= 0.0 ? 1.0 : -1.0,
                                     p.y >= 0.0 ? 1.0 : -1.0);
    }
    return p;
}

vec3 OctDecode(vec2 f) {
    vec3 d = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    if (d.z < 0.0) {
        d.xy = (1.0 - abs(d.yx)) * vec2(d.x >= 0.0 ? 1.0 : -1.0,
                                        d.y >= 0.0 ? 1.0 : -1.0);
    }
    return normalize(d);
}

// -- Layout del atlas --------------------------------------------------------

int DdgiProbeIndex(ivec3 coords) {
    return coords.x + uDdgiCounts.x * (coords.y + uDdgiCounts.y * coords.z);
}

ivec3 DdgiProbeCoords(int index) {
    int cx = uDdgiCounts.x;
    int cy = uDdgiCounts.y;
    return ivec3(index % cx, (index / cx) % cy, index / (cx * cy));
}

vec3 DdgiProbePosition(ivec3 coords) {
    return uDdgiOrigin.xyz + vec3(coords) * uDdgiSpacing.xyz;
}

// Esquina (texel del borde superior-izquierdo) del tile de un probe.
ivec2 DdgiTileOrigin(int probeIndex, int borderedTile) {
    ivec3 c = DdgiProbeCoords(probeIndex);
    ivec2 tile = ivec2(c.x + uDdgiCounts.x * c.y, c.z);   // cols = countX*countY, rows = countZ
    return tile * borderedTile;
}

// UV normalizada para samplear el tile de un probe en una direccion octaedrica.
// Mapea oct de [-1,1]^2 al INTERIOR del tile: el borde queda para que el
// filtrado bilineal tenga vecinos correctos, no para samplearse directo.
vec2 DdgiTileUV(int probeIndex, vec2 oct, int interiorTile, int borderedTile, vec2 atlasSize) {
    ivec2 origin = DdgiTileOrigin(probeIndex, borderedTile);
    vec2  local  = (oct * 0.5 + 0.5) * float(interiorTile);     // [0, interiorTile]
    vec2  texel  = vec2(origin) + vec2(float(kDdgiBorder)) + local;
    return texel / atlasSize;
}

// -- Fade del volumen --------------------------------------------------------
// 1 adentro, 0 afuera, con una celda de transicion. Evita la costura dura en el
// borde del volumen, donde DDGI cede a IBL.
float DdgiVolumeFade(vec3 worldPos) {
    vec3 g = (worldPos - uDdgiOrigin.xyz) / uDdgiSpacing.xyz;
    vec3 d = min(g, vec3(uDdgiCounts.xyz - 1) - g);   // celdas hasta el borde, por eje
    return clamp(min(d.x, min(d.y, d.z)), 0.0, 1.0);
}

// -- Sampleo con Chebyshev ---------------------------------------------------
// El corazon de DDGI. El test de Chebyshev usa la media y la media^2 de la
// distancia guardadas por probe para estimar la probabilidad de que el punto
// sea visible desde ese probe. Es lo que mata el light leaking que arruinaba a
// todos los sistemas de probes anteriores.
vec3 SampleDdgiIrradiance(vec3 worldPos, vec3 N, vec3 V) {
    // Los dos bias empujan el punto de muestreo fuera de la superficie para que
    // no se auto-ocluya contra su propia geometria.
    vec3 p = worldPos + N * uDdgiParams0.z + V * uDdgiParams0.w;

    vec3  g    = (p - uDdgiOrigin.xyz) / uDdgiSpacing.xyz;
    ivec3 base = ivec3(floor(g));
    vec3  frac = g - vec3(base);

    vec2 irrAtlasSize  = vec2(textureSize(uDdgiIrradiance, 0));
    vec2 distAtlasSize = vec2(textureSize(uDdgiDistance, 0));

    vec3  suma      = vec3(0.0);
    float sumaPesos = 0.0;

    for (int i = 0; i < 8; ++i) {
        ivec3 offset = ivec3(i & 1, (i >> 1) & 1, (i >> 2) & 1);
        ivec3 coords = clamp(base + offset, ivec3(0), uDdgiCounts.xyz - 1);

        vec3 probePos = DdgiProbePosition(coords);

        // Peso trilineal. Con el clamp de arriba, un probe repetido en el borde
        // suma su peso dos veces, que es el comportamiento que queremos: la
        // extrapolacion fuera del volumen se aplana en vez de irse al infinito.
        vec3  tri = mix(1.0 - frac, frac, vec3(offset));
        float w   = tri.x * tri.y * tri.z;

        // Rechazo suave de backface: un probe que esta "detras" de la superficie
        // no puede iluminarla. Suave y no binario para que no aparezcan
        // discontinuidades donde el signo cambia.
        vec3 dirAlProbe = normalize(probePos - p);
        w *= pow(max(dot(dirAlProbe, N) * 0.5 + 0.5, 0.0), 2.0) + 0.2;

        // Chebyshev: probabilidad de visibilidad desde este probe.
        vec3  dir  = p - probePos;
        float dist = length(dir);
        if (dist > 0.0001) {
            dir /= dist;

            vec2  momentos = texture(uDdgiDistance,
                                     DdgiTileUV(DdgiProbeIndex(coords), OctEncode(dir),
                                                uDdgiAtlas.w, uDdgiAtlas.w + 2 * kDdgiBorder,
                                                distAtlasSize)).rg;
            float media  = momentos.x;
            float media2 = momentos.y;

            if (dist > media) {
                float varianza = max(media2 - media * media, 0.0);
                float delta    = dist - media;
                float cheb     = varianza / (varianza + delta * delta);
                w *= pow(max(cheb, 0.0), uDdgiParams1.y);
            }
        }

        if (w < 0.0001) continue;

        vec3 irr = texture(uDdgiIrradiance,
                           DdgiTileUV(DdgiProbeIndex(coords), OctEncode(N),
                                      uDdgiAtlas.z, uDdgiAtlas.z + 2 * kDdgiBorder,
                                      irrAtlasSize)).rgb;

        suma      += irr * w;
        sumaPesos += w;
    }

    if (sumaPesos <= 0.0) return vec3(0.0);   // los 8 probes rechazados
    return suma / sumaPesos;
}
