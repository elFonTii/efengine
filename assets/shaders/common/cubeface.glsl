// assets/shaders/common/cubeface.glsl
// LA tabla cara -> direccion del repo. Antes estaba duplicada entre
// equirect_to_cube.comp e irradiance_convolve.comp; ahora la incluyen los dos,
// mas los blends de DDGI, que tienen que reconstruir la direccion de cada texel
// del target de captura con EXACTAMENTE la misma convencion que uso el lookAt
// que rasterizo esa cara.
//
// Es la convencion estandar de cubemaps de OpenGL: para cada cara, el mapeo
// (sc, tc, ma) del spec. uv es el centro del texel remapeado a [-1,1], y en
// uv = (0,0) cada cara devuelve su eje mayor.
//
// Los 6 pares (forward, up) que le corresponden viven en
// src/efengine/renderer/CubeFaces.h y tienen un test que los ata a esta tabla.
// Si tocas esta tabla, ese test es lo que se tiene que romper.

vec3 dirForFace(uint face, vec2 uv) {
    vec3 d;
    if      (face == 0u) d = vec3( 1.0, -uv.y, -uv.x); // +X
    else if (face == 1u) d = vec3(-1.0, -uv.y,  uv.x); // -X
    else if (face == 2u) d = vec3( uv.x,  1.0,  uv.y); // +Y
    else if (face == 3u) d = vec3( uv.x, -1.0, -uv.y); // -Y
    else if (face == 4u) d = vec3( uv.x, -uv.y,  1.0); // +Z
    else                 d = vec3(-uv.x, -uv.y, -1.0); // -Z
    return normalize(d);
}
