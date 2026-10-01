// assets/shaders/ddgi/update.glsl
// Lo que comparten los pases que actualizan probes: schedule, trace_voxel,
// probe_update y los dos blends. pbr.frag no lo incluye. Requiere ddgi/common.glsl antes.

// Espeja DdgiUpdateBlock de ShaderBlocks.h, que lleva los asserts de offsets.
layout(std140, binding = 9) uniform DdgiUpdate {
    mat4  uRayRotation;
    uvec4 uUpdateCounts;   // x = rayos por probe, y = presupuesto, z = K, w = clasificacion
    vec4  uUpdateHyst;     // x = hMax, y = umbral de irradiancia, z = umbral de brillo, w = distanceClamp
    vec4  uUpdateReloc;    // x = minFrontfaceDistance, y = reubicacion, z = voxelSize, w = backfaceFadeEnd
    vec4  uVoxelGrid;      // xyz = esquina minima, w = resolucion
    vec4  uVoxelParams;    // x = voxelSize, y = umbral de opacidad
};

// kMaxRaysPerProbe y kRayGroupSize de DdgiVolume.h; los atan static_asserts en DdgiPass.cpp.
const int   kDdgiMaxRays      = 256;
const uint  kDdgiRayGroup     = 64u;
const float kDdgiSkyDistance  = 1.0e4;   // lo que escribe un rayo que se escapa
const float kDdgiSkyThreshold = 5.0e3;

int DdgiRayCount() { return int(uUpdateCounts.x); }

// Variante de RTXGI. Areas iguales: el blend pesa solo por coseno.
vec3 DdgiSphericalFibonacci(int i, int n) {
    const float b  = 0.618033988749895;
    float phi      = 2.0 * kDdgiPI * fract(float(i) * b);
    float cosTheta = 1.0 - (2.0 * float(i) + 1.0) / float(n);
    float sinTheta = sqrt(clamp(1.0 - cosTheta * cosTheta, 0.0, 1.0));
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

// LA direccion del rayo i: la usan el trazado, la reubicacion y los dos blends. Si
// se separan, la integral se hace sobre direcciones que nadie trazo.
vec3 DdgiRayDirection(int i) {
    return normalize(mat3(uRayRotation) * DdgiSphericalFibonacci(i, DdgiRayCount()));
}

// x = edad (actualizaciones desde el ultimo reset), y = estado (0 sin dato, 1 activa,
// 2 inactiva). Lo escribe probe_update.
#ifdef DDGI_PROBE_STATE_WRITABLE
layout(std430, binding = 5) buffer DdgiProbeState {
#else
layout(std430, binding = 5) readonly buffer DdgiProbeState {
#endif
    uvec4 uDdgiProbeState[];
};

// Espejo de EffectiveHysteresis (DdgiVolume.cpp).
float DdgiEffectiveHysteresis(uint edad) {
    if (edad < 2u) return 0.0;
    return min(uUpdateHyst.x, float(edad - 1u) / float(edad));
}

int DdgiSlotCount()         { return uDdgiRange.y; }
int DdgiSlotProbe(int slot) { return (uDdgiRange.x + slot) % uDdgiCounts.w; }
