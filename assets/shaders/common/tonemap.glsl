// assets/shaders/common/tonemap.glsl
// EL tone mapping del repo: HDR lineal -> LDR sRGB. Ocurre UNA sola vez en todo
// el frame, en el composite del bloom (assets/shaders/bloom_composite.frag).
//
// Vive en un include propio y no dentro de ese shader porque el punto donde se
// tonemapea es una decision del pipeline, no del bloom: el dia que aparezca otro
// consumidor -- un blit de debug que quiera ver la imagen como se ve, un
// exportador de screenshots -- tiene que usar EXACTAMENTE esta curva y esta
// gamma, o la referencia deja de ser referencia.

// ACES filmic tone mapping -- aproximacion de Narkowicz.
// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
// Comprime el rango HDR a [0,1] con un "shoulder" que doma los highlights sin
// quemarlos de golpe (a diferencia del Reinhard puro que habia antes).
vec3 ACESFilmic(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Exposicion + curva + gamma, en ese orden. La gamma va al final y una sola vez:
// aplicarla dos veces, o antes de la curva, lava la imagen entera.
vec3 TonemapToSrgb(vec3 hdr, float exposure) {
    vec3 ldr = ACESFilmic(hdr * exposure);
    return pow(ldr, vec3(1.0 / 2.2));
}
