#version 450 core

// Blur bilateral separable del AO, guiado por la profundidad del prepass. El
// mismo idioma que el blur del bloom, con el peso de profundidad agregado para
// no cruzar siluetas: borronear a traves de un borde de objeto arrastra la
// oclusion de un objeto sobre el otro.
//
// Trabaja en TEXELS y no en UV. Cuando el AO corre a resolucion reducida hay dos
// grillas en juego -- la del target (donde estan los taps) y la del prepass, que
// siempre esta a la completa (donde esta la guia) --, y expresar el paso como un
// offset de UV obliga a acertarle a cual de las dos escala. Con texelFetch el
// paso es "un texel del target" y la guia es "texel del target por la escala",
// que es exactamente el mismo contrato que usan gtao.frag y el upsample de
// pbr.frag.

out vec4 FragColor;

layout(binding = 0) uniform sampler2D uAo;           // xyz = bent normal world, w = visibilidad
layout(binding = 1) uniform sampler2D uDepthNormal;  // FULL res: w = viewZ lineal

layout(std140, binding = 4) uniform AoPassParams {
    mat4  uViewToWorld;
    vec4  uProjInfo;
    vec4  uParams0;
    vec4  uParams1;     // y = escala del target respecto de la resolucion completa
    ivec4 uCounts;      // z = 0 horizontal, 1 vertical
};

void main() {
    ivec2 c    = ivec2(gl_FragCoord.xy);
    ivec2 paso = (uCounts.z == 0) ? ivec2(1, 0) : ivec2(0, 1);

    int   esc = int(uParams1.y + 0.5);
    ivec2 lim = textureSize(uAo, 0) - ivec2(1);

    float zCentro = texelFetch(uDepthNormal, c * esc, 0).w;
    if (zCentro <= 0.0) { FragColor = texelFetch(uAo, c, 0); return; }

    // 5 taps con pesos gaussianos 1 4 6 4 1.
    const float pesos[5] = float[5](1.0, 4.0, 6.0, 4.0, 1.0);

    vec4  suma  = vec4(0.0);
    float total = 0.0;

    for (int i = 0; i < 5; ++i) {
        // El clamp evita que el tap de los bordes envuelva al otro lado de la
        // fila. Antes lo tapaba el ClampToEdge del sampler; con texelFetch el
        // wrap del sampler no participa y el indice fuera de rango es
        // comportamiento indefinido, no un clamp.
        ivec2 t = clamp(c + paso * (i - 2), ivec2(0), lim);
        float z = texelFetch(uDepthNormal, t * esc, 0).w;
        if (z <= 0.0) continue;

        // Corte relativo y no absoluto: un umbral en metros que sirve a 2 m de
        // la camara borra todo el detalle a 50 m.
        float w = pesos[i] * step(abs(z - zCentro), zCentro * 0.05);

        suma  += texelFetch(uAo, t, 0) * w;
        total += w;
    }

    if (total <= 0.0) { FragColor = texelFetch(uAo, c, 0); return; }

    vec4 r = suma / total;
    // El bent normal promediado se renormaliza; el guard evita el NaN del caso
    // en que los taps se cancelan entre si.
    vec3 bent = (dot(r.xyz, r.xyz) > 1e-8) ? normalize(r.xyz) : r.xyz;
    FragColor = vec4(bent, r.w);
}
