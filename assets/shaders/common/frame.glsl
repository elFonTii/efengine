// assets/shaders/common/frame.glsl
// El bloque Frame (binding 0). UNICA declaracion: espejo de renderer::FrameBlock
// (ShaderBlocks.h). Lo sube Renderer::BeginScene una vez por frame.
layout(std140, binding = 0) uniform Frame {
    mat4  uView;
    mat4  uProjection;            // con jitter: la que rasteriza
    mat4  uLightSpaceMatrix;
    mat4  uInvViewProjRot;        // inverse(projection * mat4(mat3(view)))
    vec4  uViewPos;               // .xyz
    vec4  uShadowParams;          // x=enabled, y=biasMin, z=biasMax, w=normalOffset (m)
    vec4  uIblParams;             // x=hasIbl, y=intensity, z=prefilterMaxLod
    mat4  uInvView;
    mat4  uInvProjection;
    mat4  uViewProjNoJitter;
    mat4  uPrevViewProjNoJitter;
    vec4  uJitter;                // xy = jitter NDC de este frame, zw = el del anterior
    vec4  uScreen;                // x = ancho, y = alto, z = 1/ancho, w = 1/alto
    uvec4 uFrameParams;           // x = frameIndex, y = jitter prendido
};
