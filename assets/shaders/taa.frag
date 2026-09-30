#version 450 core
// Resolve temporal en HDR. Espejo testeable: renderer/TaaMath.cpp.
in vec2 vUV;
out vec4 FragColor;

layout(binding = 0) uniform sampler2D uCurrent;    // HDR con jitter
layout(binding = 1) uniform sampler2D uHistory;    // resolve del frame anterior
layout(binding = 2) uniform sampler2D uVelocity;   // RG16F del prepass
layout(binding = 3) uniform sampler2D uDepth;      // D32F

layout(std140, binding = 4) uniform TaaParams {
    mat4 uInvViewProjNoJitter;
    mat4 uPrevViewProjNoJitter;
    vec4 uJitterUv;   // xy
    vec4 uScreen;     // ancho, alto, 1/ancho, 1/alto
    vec4 uParams;     // x = alfa efectivo, y = ver velocidades
};

vec3 RgbToYCoCg(vec3 c) {
    return vec3( 0.25 * c.r + 0.5 * c.g + 0.25 * c.b,
                 0.5  * c.r             - 0.5  * c.b,
                -0.25 * c.r + 0.5 * c.g - 0.25 * c.b);
}

vec3 YCoCgToRgb(vec3 c) {
    float t = c.x - c.z;
    return vec3(t + c.y, c.x + c.z, t - c.y);
}

vec3 ClipToAabb(vec3 h, vec3 mn, vec3 mx) {
    vec3  centro = 0.5 * (mx + mn);
    vec3  medio  = 0.5 * (mx - mn) + 1e-5;
    vec3  v      = h - centro;
    vec3  u      = abs(v / medio);
    float a      = max(u.x, max(u.y, u.z));
    return a > 1.0 ? centro + v / a : h;
}

float Luma(vec3 c) { return dot(c, vec3(0.2126, 0.7152, 0.0722)); }

vec2 ClipToUv(vec4 clip) { return clip.xy / clip.w * 0.5 + 0.5; }

vec2 CameraVelocityUv(vec2 uv, float depth) {
    vec2 uvSinJitter = uv - uJitterUv.xy;
    vec4 mundo = uInvViewProjNoJitter * vec4(uvSinJitter * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    mundo /= mundo.w;
    return uvSinJitter - ClipToUv(uPrevViewProjNoJitter * mundo);
}

// Catmull-Rom en 5 lecturas bilineales (Jimenez, Filmic SMAA 2016): las 4
// esquinas pesan casi nada y se omiten.
vec3 HistoryCatmullRom(vec2 uv) {
    vec2 pos   = uv * uScreen.xy;
    vec2 t1    = floor(pos - 0.5) + 0.5;
    vec2 f     = pos - t1;
    vec2 w0    = f * (-0.5 + f * (1.0 - 0.5 * f));
    vec2 w1    = 1.0 + f * f * (-2.5 + 1.5 * f);
    vec2 w2    = f * (0.5 + f * (2.0 - 1.5 * f));
    vec2 w3    = f * f * (-0.5 + 0.5 * f);
    vec2 w12   = w1 + w2;
    vec2 t0    = (t1 - 1.0) * uScreen.zw;
    vec2 t3    = (t1 + 2.0) * uScreen.zw;
    vec2 t12   = (t1 + w2 / w12) * uScreen.zw;

    vec3 r = texture(uHistory, vec2(t12.x, t0.y)).rgb  * (w12.x * w0.y)
           + texture(uHistory, vec2(t0.x,  t12.y)).rgb * (w0.x  * w12.y)
           + texture(uHistory, t12).rgb                * (w12.x * w12.y)
           + texture(uHistory, vec2(t3.x,  t12.y)).rgb * (w3.x  * w12.y)
           + texture(uHistory, vec2(t12.x, t3.y)).rgb  * (w12.x * w3.y);
    float suma = w12.x * w0.y + w0.x * w12.y + w12.x * w12.y + w3.x * w12.y + w12.x * w3.y;
    // Los lobulos negativos pueden dar valores bajo cero.
    return max(r / suma, vec3(0.0));
}

void main() {
    ivec2 p    = ivec2(gl_FragCoord.xy);
    ivec2 maxP = ivec2(uScreen.xy) - 1;
    vec3  cur  = texelFetch(uCurrent, p, 0).rgb;

    // Una sola pasada por el 3x3: caja de color y texel mas cercano (Karis 2014).
    vec3  mn    = vec3( 1e30);
    vec3  mx    = vec3(-1e30);
    float zMin  = 1.0;
    ivec2 pMin  = p;
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            ivec2 q = clamp(p + ivec2(x, y), ivec2(0), maxP);
            vec3  c = RgbToYCoCg(texelFetch(uCurrent, q, 0).rgb);
            mn = min(mn, c);
            mx = max(mx, c);
            float z = texelFetch(uDepth, q, 0).r;
            if (z < zMin) { zMin = z; pMin = q; }
        }
    }

    vec2 uv  = (vec2(p) + 0.5) * uScreen.zw;
    // Cielo: no paso por el prepass, su velocidad es solo la de la camara.
    vec2 vel = (zMin >= 1.0) ? CameraVelocityUv(uv, 1.0) : texelFetch(uVelocity, pMin, 0).xy;

    if (uParams.y > 0.5) {
        FragColor = vec4(abs(vel) * 50.0, 0.0, 1.0);
        return;
    }

    vec2  uvPrev = uv - vel;
    float alfa   = uParams.x;
    if (any(lessThan(uvPrev, vec2(0.0))) || any(greaterThan(uvPrev, vec2(1.0)))) alfa = 1.0;

    // Antes de leer la historia: sin historia valida puede tener NaN, y NaN * 0 = NaN.
    if (alfa >= 1.0) {
        FragColor = vec4(cur, 1.0);
        return;
    }

    vec3 hist = HistoryCatmullRom(uvPrev);
    hist = YCoCgToRgb(ClipToAabb(RgbToYCoCg(hist), mn, mx));

    float wCur  = alfa / (1.0 + Luma(cur));
    float wHist = (1.0 - alfa) / (1.0 + Luma(hist));
    vec3  res   = (cur * wCur + hist * wHist) / max(wCur + wHist, 1e-6);

    if (any(isnan(res)) || any(isinf(res))) res = cur;
    FragColor = vec4(res, 1.0);
}
