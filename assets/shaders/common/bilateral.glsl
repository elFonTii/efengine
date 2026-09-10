// assets/shaders/common/bilateral.glsl
// EL upsample bilateral del repo. Lo incluye pbr.frag para reconstruir a
// resolucion completa las dos señales que se calculan a media: la indirecta
// difusa de DDGI y la visibilidad + bent normal del GTAO.
//
// Por que hace falta un upsample GUIADO y no un bilineal a secas: las dos
// señales se calculan sobre un subconjunto de los pixeles, y un bilineal mezcla
// los cuatro vecinos sin mirar QUE hay en ellos. Sobre una silueta, dos de esos
// cuatro estan en el objeto de atras: la oclusion del fondo se derrama sobre el
// borde del objeto de adelante y aparece un halo de un pixel de ancho alrededor
// de toda la geometria. Es el artefacto clasico del AO a media resolucion.
//
// La guia son el depth y la normal del prepass del AO, A RESOLUCION COMPLETA.
// Con las dos y no solo con el depth: dos paredes que se tocan en un rincon
// tienen la MISMA profundidad y orientaciones opuestas, asi que un peso solo por
// profundidad las mezcla y sangra la irradiancia de una sobre la otra. El
// termino de normal es lo que corta ese caso, y el rincon de Cornell es
// exactamente donde se ve.
//
// -- El contrato de correspondencia --
// El pase a resolucion reducida samplea su guia en el texel full-res
// (h * escala) de cada texel suyo (h): la esquina superior-izquierda del bloque
// que le toca. Este upsample tiene que leer la guia EN EL MISMO TEXEL, por eso
// el `tap * escala` de abajo. Si las dos puntas no coinciden, los pesos comparan
// contra una profundidad que no es la que el tap uso y el filtro deja de ser
// bilateral: vuelve a ser un bilineal con pasos extra, halos incluidos.
//
// Lo respetan gtao.frag, ddgi/indirect.frag y ao/denoise.frag. Es LA invariante
// del sistema de resolucion reducida; si se toca, se toca en los cuatro.

// Tolerancia de profundidad, RELATIVA a la profundidad del pixel. Absoluta no
// sirve: un umbral en metros calibrado a 2 m de la camara borra todo el detalle
// a 50 m. Es el mismo criterio que el corte de ao/denoise.frag.
const float kBilateralDepthTol = 0.05;

// Exponente del peso de normal. 8 deja pasar ~10 grados de diferencia con peso
// casi pleno y mata el peso a 45: suficientemente ancho para que la
// interpolacion de normales dentro de una cara curva no se rompa en bandas, y
// suficientemente angosto para separar dos caras de un rincon.
const float kBilateralNormalPow = 8.0;

// Peso de un tap. zGuia/nGuia salen del prepass en el texel del tap; zCentro y
// nCentro son los del pixel que se esta sombreando. Todo en espacio de VISTA.
float BilateralTapWeight(float zGuia, vec3 nGuia, float zCentro, vec3 nCentro) {
    // viewZ == 0 es el centinela de "aca no hay geometria" del prepass: ese tap
    // no describe ninguna superficie y no puede pesar.
    if (zGuia <= 0.0) return 0.0;

    // Profundidad: cae linealmente hasta la tolerancia y ahi corta. Lineal y no
    // gaussiano porque lo unico que importa es "es la misma superficie o no".
    float dz = abs(zGuia - zCentro) / max(zCentro * kBilateralDepthTol, 1e-4);
    float wz = max(1.0 - dz, 0.0);

    // Normal: el coseno elevado. max(...,0) descarta el tap de una cara opuesta
    // en vez de darle peso negativo.
    float wn = pow(max(dot(nGuia, nCentro), 0.0), kBilateralNormalPow);

    return wz * wn;
}

// Reconstruye a resolucion completa el valor de una textura a media resolucion.
//
//   media      : la señal reducida (el resultado del pase que se quiere subir)
//   guia       : el prepass del AO a resolucion COMPLETA (xyz = normal view, w = viewZ)
//   fullCoord  : ivec2(gl_FragCoord.xy) del pixel que se esta sombreando
//   zCentro    : su viewZ (positivo, la misma convencion que el prepass)
//   nCentro    : su normal GEOMETRICA en espacio de vista, normalizada
//   escala     : texels de resolucion completa por texel de `media`, por eje
//
// El tamano de `media` sale de textureSize y no de un uniform: es un dato que la
// propia textura ya tiene, y pasarlo aparte crea una segunda fuente de verdad
// que se desincroniza justo en el frame del resize.
//
// El fallback cuando los cuatro taps se rechazan (silueta fina, un pixel de
// geometria contra el fondo) es el tap de profundidad mas parecida, sin
// interpolar. Es una muestra de media resolucion puesta cruda en un pixel de
// full: se ve como un escalon de un pixel, que es MUCHO menos visible que el
// halo que produciria promediar los cuatro igual.
vec4 BilateralUpsample(sampler2D media, sampler2D guia, ivec2 fullCoord,
                       float zCentro, vec3 nCentro, int escala) {
    ivec2 mediaSize = textureSize(media, 0);

    // Escala 1: la señal ya esta a resolucion completa, no hay nada que subir y
    // los cuatro taps colapsarian sobre el mismo texel con pesos bilineales
    // degenerados. Un texelFetch y afuera.
    if (escala <= 1) return texelFetch(media, clamp(fullCoord, ivec2(0), mediaSize - 1), 0);

    // Centro del pixel full-res en coordenadas de TEXEL de la grilla reducida,
    // corrido medio texel: asi floor() da la esquina del quad 2x2 de taps y la
    // parte fraccionaria son los pesos bilineales.
    vec2  h    = (vec2(fullCoord) + 0.5) / float(escala) - 0.5;
    ivec2 base = ivec2(floor(h));
    vec2  f    = h - vec2(base);

    vec4  suma  = vec4(0.0);
    float total = 0.0;

    // Fallback: el tap cuya profundidad de guia mas se parece a la del centro.
    vec4  mejor     = vec4(0.0);
    float mejorDiff = 1e30;

    for (int i = 0; i < 4; ++i) {
        ivec2 tap = clamp(base + ivec2(i & 1, i >> 1), ivec2(0), mediaSize - 1);

        // La guia se lee en el texel full-res que el pase reducido uso. Ver el
        // contrato de correspondencia arriba.
        vec4  g     = texelFetch(guia, tap * escala, 0);
        vec4  valor = texelFetch(media, tap, 0);

        // Peso bilineal del tap dentro del quad.
        vec2  bw = mix(1.0 - f, f, vec2(i & 1, i >> 1));
        float w  = bw.x * bw.y * BilateralTapWeight(g.w, normalize(g.xyz), zCentro, nCentro);

        suma  += valor * w;
        total += w;

        float diff = (g.w > 0.0) ? abs(g.w - zCentro) : 1e29;
        if (diff < mejorDiff) { mejorDiff = diff; mejor = valor; }
    }

    // El corte no es `total > 0`: con pesos del orden de 1e-8 la division
    // amplifica el ruido del unico tap que sobrevivio hasta hacerlo visible.
    return (total > 1e-4) ? suma / total : mejor;
}
