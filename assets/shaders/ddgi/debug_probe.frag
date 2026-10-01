#version 450 core
// Muestra el tile de un probe sobre la superficie de su esfera. Es el
// instrumento que hace debuggeable DDGI: el modo 1 (distancia) es como se ve
// si el atlas de Chebyshev esta mal, que de otra forma solo se manifiesta como
// "hay leaking" sin decir donde.

in vec3 vNormal;

out vec4 FragColor;

layout(std140, binding = 4) uniform PassParams {
    vec4 uProbeParams;   // x = probeIndex, y = modo (0=irradiancia, 1=distancia)
};

#include "ddgi/common.glsl"

void main() {
    int  probe = int(uProbeParams.x + 0.5);
    vec3 dir   = normalize(vNormal);
    vec2 oct   = OctEncode(dir);

    if (uProbeParams.y > 0.5) {
        // Normalizada contra el clamp de distancia (params2.x).
        vec2  atlasSize = vec2(textureSize(uDdgiDistance, 0));
        float media = texture(uDdgiDistance,
                              DdgiTileUV(probe, oct, uDdgiAtlas.w,
                                         uDdgiAtlas.w + 2 * kDdgiBorder, atlasSize)).r;
        float escala = uDdgiParams2.x;
        FragColor = vec4(vec3(clamp(media / escala, 0.0, 1.0)), 1.0);
    } else {
        vec2 atlasSize = vec2(textureSize(uDdgiIrradiance, 0));
        vec3 irr = texture(uDdgiIrradiance,
                           DdgiTileUV(probe, oct, uDdgiAtlas.z,
                                      uDdgiAtlas.z + 2 * kDdgiBorder, atlasSize)).rgb;
        FragColor = vec4(irr, 1.0);
    }

    // Rojo = inactivo por clasificacion.
    FragColor.rgb = mix(FragColor.rgb, vec3(1.0, 0.0, 0.0), 1.0 - DdgiProbeWeight(probe));
}
