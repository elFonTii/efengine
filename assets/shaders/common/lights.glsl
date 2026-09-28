// assets/shaders/common/lights.glsl
// Las luces del frame y su matematica, una sola fuente para pbr.frag,
// ddgi/trace_voxel.comp, phong.frag y lights/cluster_cull.comp: el forward y
// la GI no pueden calcular distinto la misma luz.
//
// Espeja LightsBlock y GpuLight de renderer/ShaderBlocks.h y las funciones de
// renderer/LightMath.h, que es donde se testean.

layout(std140, binding = 1) uniform Lights {
    vec4  uDirDirection[4];   // xyz = hacia donde VIAJA la luz
    vec4  uDirColor[4];       // rgb = color efectivo, w = 1 si es el PrimarySun
    uvec4 uLightCounts;       // x = locales, y = direccionales, z = visibles
};

struct GpuLight {
    vec4 positionRange;       // xyz = mundo, w = rango (m)
    vec4 colorRadius;         // rgb = color efectivo, w = radio de la fuente (m)
    vec4 directionType;       // xyz = direccion del spot, w = tipo (0 point, 1 spot)
    vec4 spotParams;          // x = scale, y = offset, z = cos exterior, w = sin exterior
    vec4 reserved;            // x = shadowIndex, y = flags: ciclo 2
};

layout(std430, binding = 1) readonly buffer LocalLights {
    GpuLight uLocalLights[];
};

layout(std430, binding = 2) readonly buffer VisibleLights {
    uint uVisibleLights[];
};

// Ventana de Karis (2013, ec. 9) sobre inverse-square, con el piso de
// Frostbite en el denominador. d2 = distancia al cuadrado.
float LightFalloff(float d2, float range, float sourceRadius) {
    float ratio = d2 / (range * range);
    float win   = clamp(1.0 - ratio * ratio, 0.0, 1.0);
    float rMin  = max(sourceRadius, 0.01);
    return (win * win) / max(d2, rMin * rMin);
}

// l = de la superficie a la luz, normalizada.
float SpotAngular(GpuLight luz, vec3 l) {
    if (luz.directionType.w < 0.5) return 1.0;
    float t = clamp(dot(luz.directionType.xyz, -l) * luz.spotParams.x + luz.spotParams.y, 0.0, 1.0);
    return t * t;
}

// Karis ec. 11: el punto de la esfera mas cercano al rayo reflejado.
// Lvec = de la superficie al centro de la luz, sin normalizar.
vec3 RepresentativePoint(vec3 Lvec, vec3 R, float sourceRadius) {
    vec3 centerToRay = dot(Lvec, R) * R - Lvec;
    vec3 closest     = Lvec + centerToRay * clamp(sourceRadius / max(length(centerToRay), 1e-6), 0.0, 1.0);
    return normalize(closest);
}

// Karis ec. 10 y 14: el lobulo se ensancha por el angulo de la fuente y la
// energia se renormaliza para que una luz grande no brille de mas.
float SphereNormalization(float roughness, float sourceRadius, float d) {
    float a      = roughness * roughness;
    float aPrima = clamp(a + sourceRadius / (2.0 * max(d, 1e-4)), 0.0, 1.0);
    float k      = a / max(aPrima, 1e-4);
    return k * k;
}
