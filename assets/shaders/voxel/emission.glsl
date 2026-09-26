// assets/shaders/voxel/emission.glsl
// Emision del voxel en RGBM sobre un RGBA8: rgb = e / M, a = sqrt(M / kMax).
// La incluyen voxel/voxelize.frag, que codifica, y ddgi/trace_voxel.comp, que
// decodifica: si se separan, la GI emisiva cambia de brillo sin que nada falle.
//
// kMax = 64 cubre el slider de intensidad (0-20) con margen para el valor
// tipeado a mano; lo que pase de 64 se satura conservando el tono. La raiz da
// mas pasos de 8 bits a la emision baja: la simulacion en Node dio < 0.4% de
// error relativo en [0.01, 64], contra ~4.8% de la curva lineal.

const float kVoxelEmissionMax = 64.0;

vec4 EncodeVoxelEmission(vec3 e) {
    float m = max(max(e.r, e.g), e.b);
    if (m <= 0.0) return vec4(0.0);
    // a se cuantiza hacia ARRIBA: asi M >= m y rgb no pasa de 1.
    float a = min(ceil(sqrt(min(m / kVoxelEmissionMax, 1.0)) * 255.0) / 255.0, 1.0);
    return vec4(clamp(e / (a * a * kVoxelEmissionMax), 0.0, 1.0), a);
}

vec3 DecodeVoxelEmission(vec4 t) {
    return t.rgb * (t.a * t.a * kVoxelEmissionMax);
}
