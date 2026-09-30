// assets/shaders/common/object.glsl
// El bloque Object (binding 2). UNICA declaracion: espejo de renderer::ObjectBlock.
layout(std140, binding = 2) uniform Object {
    mat4 uModel;
    mat4 uPrevModel;   // solo lo lee el prepass
};
