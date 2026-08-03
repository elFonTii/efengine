#version 450 core

// Blur bilateral separable del AO, guiado por la profundidad del prepass. El
// mismo idioma que el blur del bloom, con el peso de profundidad agregado para
// no cruzar siluetas: borronear a traves de un borde de objeto arrastra la
// oclusion de un objeto sobre el otro.

in vec2 vUV;
out vec4 FragColor;

layout(binding = 0) uniform sampler2D uAo;           // xyz = bent normal world, w = visibilidad
layout(binding = 1) uniform sampler2D uDepthNormal;  // w = viewZ lineal

layout(std140, binding = 4) uniform AoPassParams {
    mat4  uViewToWorld;
    vec4  uProjInfo;    // zw = 1/resolucion
    vec4  uParams0;
    vec4  uParams1;
    ivec4 uCounts;      // z = 0 horizontal, 1 vertical
};

void main() {
    vec2 paso = (uCounts.z == 0) ? vec2(uProjInfo.z, 0.0) : vec2(0.0, uProjInfo.w);

    float zCentro = texture(uDepthNormal, vUV).w;
    if (zCentro <= 0.0) { FragColor = texture(uAo, vUV); return; }

    // 5 taps con pesos gaussianos 1 4 6 4 1.
    const float pesos[5] = float[5](1.0, 4.0, 6.0, 4.0, 1.0);

    vec4  suma  = vec4(0.0);
    float total = 0.0;

    for (int i = 0; i < 5; ++i) {
        vec2  uv = vUV + paso * float(i - 2);
        float z  = texture(uDepthNormal, uv).w;
        if (z <= 0.0) continue;

        // Corte relativo y no absoluto: un umbral en metros que sirve a 2 m de
        // la camara borra todo el detalle a 50 m.
        float w = pesos[i] * step(abs(z - zCentro), zCentro * 0.05);

        suma  += texture(uAo, uv) * w;
        total += w;
    }

    if (total <= 0.0) { FragColor = texture(uAo, vUV); return; }

    vec4 r = suma / total;
    // El bent normal promediado se renormaliza; el guard evita el NaN del caso
    // en que los taps se cancelan entre si.
    vec3 bent = (dot(r.xyz, r.xyz) > 1e-8) ? normalize(r.xyz) : r.xyz;
    FragColor = vec4(bent, r.w);
}
