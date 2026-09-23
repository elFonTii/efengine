#version 450 core
// Estampa el fragmento en el voxel que le toca. No escribe color ni
// profundidad: el unico efecto son los dos imageStore.

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;

layout(rgba8, binding = 0) uniform image3D uVoxelAlbedo;
layout(rg8,   binding = 1) uniform image3D uVoxelNormal;

layout(binding = 0) uniform sampler2D uAlbedoMap;

layout(std140, binding = 3) uniform MaterialParams {
    vec4  uAlbedoTint;
    vec4  uEmissiveTint;
    vec4  uScalars0;
    vec4  uScalars1;
    uvec4 uMapMask;
    vec4  uUvTransform;
};

layout(std140, binding = 4) uniform PassParams {
    mat4 uViewProj;
    vec4 uGridOrigin;   // .xyz
    vec4 uGridParams;   // x = voxelSize (m), y = resolucion por eje
};

const uint SLOT_ALBEDO = 0u;

bool hasMap(uint slot) { return (uMapMask.x & (1u << slot)) != 0u; }

vec2 OctEncodeNormal(vec3 n) {
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    vec2 p = n.xy;
    if (n.z < 0.0) {
        p = (1.0 - abs(p.yx)) * vec2(p.x >= 0.0 ? 1.0 : -1.0,
                                     p.y >= 0.0 ? 1.0 : -1.0);
    }
    return p * 0.5 + 0.5;   // a [0,1] para RG8
}

void main() {
    int resolucion = int(uGridParams.y);
    ivec3 voxel = ivec3(floor((vWorldPos - uGridOrigin.xyz) / uGridParams.x));
    if (any(lessThan(voxel, ivec3(0))) || any(greaterThanEqual(voxel, ivec3(resolucion)))) {
        return;
    }

    vec2 uv = vUV * uUvTransform.xy + uUvTransform.zw;
    vec3 albedo = hasMap(SLOT_ALBEDO)
                ? texture(uAlbedoMap, uv).rgb * uAlbedoTint.rgb
                : uAlbedoTint.rgb;

    // Alfa en 1: este voxel esta ocupado. Las colisiones las gana el ultimo que
    // escribe, y esta bien: dos superficies distintas en el mismo voxel ya
    // perdieron la distincion.
    imageStore(uVoxelAlbedo, voxel, vec4(albedo, 1.0));
    imageStore(uVoxelNormal, voxel, vec4(OctEncodeNormal(normalize(vNormal)), 0.0, 0.0));
}
