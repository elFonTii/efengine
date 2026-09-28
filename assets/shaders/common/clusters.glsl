// assets/shaders/common/clusters.glsl
// La grilla de clusters: el bloque, las listas y el indice. Espeja
// renderer/ClusterMath.h (ClusterSlice, ClusterIndex) y ClusterBlock de
// renderer/ShaderBlocks.h, que es donde se testean.

layout(std140, binding = 8) uniform Clusters {
    uvec4 uClusterDims;     // x = tilesX, y = tilesY, z = cortes, w = max por cluster (0 = sin grilla)
    vec4  uClusterZ;        // x = nearSplit, y = farLimit, z = (cortes-1)/ln(farLimit/nearSplit), w = ln(nearSplit)
    vec4  uClusterScreen;   // x = tile en px, y = vista de debug
};

// Por cluster: [cantidad, indice0, indice1, ...] con paso max + 1. Los indices
// son de LocalLights.
#ifdef CLUSTER_LIGHTS_WRITABLE
layout(std430, binding = 3) writeonly buffer ClusterLights {
#else
layout(std430, binding = 3) readonly buffer ClusterLights {
#endif
    uint uClusterLights[];
};

bool ClustersEnabled() { return uClusterDims.w > 0u; }
uint ClusterStride()   { return uClusterDims.w + 1u; }

uint ClusterSlice(float viewZ) {
    if (!(viewZ >= uClusterZ.x)) return 0u;
    float s = floor((log(viewZ) - uClusterZ.w) * uClusterZ.z);
    return min(1u + uint(min(max(s, 0.0), float(uClusterDims.z))), uClusterDims.z - 1u);
}

uint ClusterIndex(vec2 fragCoord, float viewZ) {
    uvec2 tile = min(uvec2(fragCoord / uClusterScreen.x), uClusterDims.xy - 1u);
    return (ClusterSlice(viewZ) * uClusterDims.y + tile.y) * uClusterDims.x + tile.x;
}
