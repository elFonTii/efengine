#version 450 core

// Resuelve la irradiancia indirecta de DDGI A MEDIA RESOLUCION, en un target
// aparte. pbr.frag la sube a resolucion completa con un upsample bilateral en
// vez de samplear el volumen por pixel.
//
// Por que existe este pase: el sampleo de DDGI son ~16 gathers por pixel con
// coordenadas calculadas por pixel (8 probes trilineales x lookup octaedrico de
// irradiancia + momentos de distancia). Las direcciones no son predecibles, no
// hay prefetch posible y la cache de texturas se satura. Medido en el Forward a
// 1080p daba 1.892 ms con SEIS draws -- ~9.300 ciclos de shader por pixel,
// 10-20x lo que explica la aritmetica del PBR. A un cuarto de los pixeles, son
// un cuarto de los gathers.
//
// Por que se PUEDE bajar de resolucion: la indirecta difusa es de baja
// frecuencia espacial por construccion. La grilla de probes ya la muestrea cada
// varios metros y la interpolacion trilineal la suaviza entre ellos; lo que
// tiene alto detalle en la imagen final es el albedo y el AO de contacto, y
// esos dos se siguen aplicando a resolucion completa en pbr.frag.
//
// La geometria NO se re-rasteriza: la posicion, la normal y el bent normal
// salen del prepass del AO y del propio target del AO, que ya existen y ya
// corrieron este frame. Por eso este pase depende de que el AO este encendido;
// si no lo esta, pbr.frag vuelve al camino inline (ver IndirectPass.h).

in vec2 vUV;
out vec4 FragColor;   // rgb = irradiancia indirecta ya escalada; a = fade del volumen

layout(binding = 0) uniform sampler2D uDepthNormal;   // prepass AO, FULL res: xyz = normal view, w = viewZ
layout(binding = 1) uniform sampler2D uAo;            // target AO: xyz = bent normal WORLD, w = visibilidad

// El bloque Frame de la camara (binding 0). Este pase corre DESPUES de
// BeginScene a proposito -- ver el comentario de orden en IndirectPass.h --, asi
// que la view/proj y la posicion de camara del frame ya estan subidas y no hace
// falta re-empaquetarlas.
layout(std140, binding = 0) uniform Frame {
    mat4 uView;
    mat4 uProjection;
    mat4 uLightSpaceMatrix;
    mat4 uInvViewProjRot;
    vec4 uViewPos;        // .xyz = camara
    vec4 uShadowParams;
    vec4 uIblParams;
};

layout(std140, binding = 4) uniform IndirectParams {
    mat4  uViewToWorld;   // inverse(view): del prepass (view-space) al mundo
    vec4  uProjInfo;      // xy = reconstruccion view-space, zw = 1/resolucion COMPLETA
    ivec4 uCounts;        // x = escala vs full, y = origen del bent normal, zw = tamano full
};

// De donde sale el bent normal. No es un bool: el target del AO puede estar a
// resolucion completa (mientras el AO no baje de resolucion) o compartir la de
// este pase, y leer en la grilla equivocada devuelve el bent normal de otro
// pixel -- un error que no rompe nada ruidosamente, solo tuerce la direccion
// del color bleeding.
const int kBentNinguno  = 0;   // usar la normal geometrica
const int kBentFullRes  = 1;   // el target del AO esta a resolucion completa
const int kBentReducido = 2;   // el target del AO comparte la resolucion de este pase

// El bloque DDGI (binding 5), los atlas en 12/13 y toda la matematica.
#include "ddgi/common.glsl"

vec3 ViewPos(vec2 uv, float viewZ) {
    vec2 ndc = uv * 2.0 - 1.0;
    return vec3(ndc * uProjInfo.xy, -1.0) * viewZ;
}

void main() {
    // -- El contrato de correspondencia con el upsample -----------------------
    // Se lee el prepass EN EL TEXEL full-res (2*h), no con texture() en el
    // centro del pixel de media. Dos motivos, y los dos importan:
    //
    //  1. texture() en el centro de un pixel de media res cae justo entre
    //     cuatro texels de full: el filtrado bilineal devuelve el PROMEDIO de
    //     las cuatro profundidades. Sobre una silueta ese promedio no es la
    //     profundidad de ninguna superficie real, y la irradiancia se calcula
    //     para un punto que no existe.
    //
    //  2. common/bilateral.glsl busca la guia de cada tap en `tap * 2`. Si este
    //     pase muestrea en otro lado, los pesos comparan contra una superficie
    //     distinta de la que el tap uso y el filtro deja de ser bilateral.
    ivec2 full = ivec2(gl_FragCoord.xy) * uCounts.x;
    vec4  g    = texelFetch(uDepthNormal, full, 0);
    float viewZ = g.w;

    // Cielo: no hay superficie que iluminar. Fade 0 hace que pbr.frag se quede
    // con el IBL, que es lo correcto para el fondo.
    if (viewZ <= 0.0) { FragColor = vec4(0.0); return; }

    // UV del texel de guia, no del pixel de media: la reconstruccion de posicion
    // tiene que usar la MISMA coordenada que la profundidad que reconstruye.
    vec2 uvFull = (vec2(full) + 0.5) * uProjInfo.zw;

    vec3 Nview    = normalize(g.xyz);
    vec3 worldPos = (uViewToWorld * vec4(ViewPos(uvFull, viewZ), 1.0)).xyz;
    vec3 N        = normalize(mat3(uViewToWorld) * Nview);
    vec3 V        = normalize(uViewPos.xyz - worldPos);

    // El bent normal sale del target del AO, que ya esta en espacio MUNDO. En
    // que texel se lee depende de la resolucion de ESE target, no de la de este
    // pase: ver las constantes kBent* arriba.
    vec3 bentN = N;
    if (uCounts.y != kBentNinguno) {
        ivec2 c = (uCounts.y == kBentReducido) ? ivec2(gl_FragCoord.xy) : full;
        vec3 crudo = texelFetch(uAo, c, 0).xyz;
        if (dot(crudo, crudo) > 1e-8) bentN = normalize(crudo);
    }

    // Con DDGI apagado el fade tiene que salir en CERO, no el del volumen. El
    // fade es el peso con el que pbr.frag mezcla contra el IBL: dejarlo en 1
    // adentro del volumen y la irradiancia en negro apagaria el ambiente entero
    // ahi adentro en vez de caer a IBL puro, que es lo que hacia el camino
    // inline cuando el usuario destildaba "Habilitado" en el panel.
    float fade = DdgiEnabled() ? DdgiVolumeFade(worldPos) : 0.0;

    // Misma condicion que tenia pbr.frag: con fade 0 el sampleo no va a ningun
    // lado, salvo en el modo de debug que existe justamente para ver lo que el
    // fade descarta.
    vec3 irr = vec3(0.0);
    if (DdgiEnabled() && (fade > 0.0 || DdgiDebugView() == kDdgiViewDdgiNoFade)) {
        irr = (DdgiAblate() ? vec3(DdgiAblateIrradiance())
                            : SampleDdgiIrradiance(worldPos, N, bentN, V))
            * uDdgiParams0.y;
    }

    // La intensidad artistica ya esta aplicada: pbr.frag recibe el termino listo
    // para mezclarlo con el IBL segun el fade del alfa.
    FragColor = vec4(irr, fade);
}
