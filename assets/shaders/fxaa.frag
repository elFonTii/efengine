#version 450 core

// FXAA (Fast Approximate Anti-Aliasing) — variante compacta de Timothy Lottes
// (la "console" FXAA popularizada por NVIDIA / geeks3d). Opera sobre la imagen
// LDR ya tonemapeada y gamma-encoded, que es el espacio perceptual donde FXAA
// funciona bien. Detecta bordes por contraste de luma y borronea a lo largo
// del borde (no a través de él), suavizando el dentado sin perder nitidez.

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uScreenTexture;   // imagen LDR gamma-encoded (salida del tonemap)
uniform bool      uEnabled;         // off → passthrough, para comparar on/off

// Tunables clásicos de la variante compacta.
const float SPAN_MAX   = 8.0;         // largo máximo del muestreo a lo largo del borde
const float REDUCE_MUL = 1.0 / 8.0;   // atenúa la dirección en zonas brillantes
const float REDUCE_MIN = 1.0 / 128.0; // piso para no dividir por ~0

// Luma perceptual (coeficientes Rec.601, estándar en FXAA).
float luma(vec3 rgb) { return dot(rgb, vec3(0.299, 0.587, 0.114)); }

void main() {
    vec2 texel = 1.0 / vec2(textureSize(uScreenTexture, 0));   // resolución-agnóstico
    vec3 rgbM  = texture(uScreenTexture, vUV).rgb;

    if (!uEnabled) { FragColor = vec4(rgbM, 1.0); return; }

    // Luma del centro y de los 4 vecinos diagonales.
    float lM  = luma(rgbM);
    float lNW = luma(texture(uScreenTexture, vUV + vec2(-1.0, -1.0) * texel).rgb);
    float lNE = luma(texture(uScreenTexture, vUV + vec2( 1.0, -1.0) * texel).rgb);
    float lSW = luma(texture(uScreenTexture, vUV + vec2(-1.0,  1.0) * texel).rgb);
    float lSE = luma(texture(uScreenTexture, vUV + vec2( 1.0,  1.0) * texel).rgb);

    float lMin = min(lM, min(min(lNW, lNE), min(lSW, lSE)));
    float lMax = max(lM, max(max(lNW, lNE), max(lSW, lSE)));

    // Dirección del borde: perpendicular al gradiente de luma diagonal.
    vec2 dir;
    dir.x = -((lNW + lNE) - (lSW + lSE));
    dir.y =  ((lNW + lSW) - (lNE + lSE));

    // Normaliza la dirección y la acota a ±SPAN_MAX téxeles.
    float dirReduce = max((lNW + lNE + lSW + lSE) * 0.25 * REDUCE_MUL, REDUCE_MIN);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcpDirMin, -SPAN_MAX, SPAN_MAX) * texel;

    // Dos muestreos estrechos (rgbA) y dos anchos (rgbB) a lo largo del borde.
    vec3 rgbA = 0.5 * (
        texture(uScreenTexture, vUV + dir * (1.0 / 3.0 - 0.5)).rgb +
        texture(uScreenTexture, vUV + dir * (2.0 / 3.0 - 0.5)).rgb);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(uScreenTexture, vUV + dir * -0.5).rgb +
        texture(uScreenTexture, vUV + dir *  0.5).rgb);

    // Si el muestreo ancho se salió del rango de luma local, sobre-borroneó →
    // usar el estrecho (rgbA); si no, el ancho (rgbB, de mejor calidad).
    float lB = luma(rgbB);
    FragColor = vec4((lB < lMin || lB > lMax) ? rgbA : rgbB, 1.0);
}
