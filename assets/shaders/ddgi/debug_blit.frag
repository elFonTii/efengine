#version 450 core
// Vuelca el buffer de rayos en la esquina inferior izquierda: una fila por probe del
// frame, una columna por rayo.
// PassParams: x = lado del recuadro (fraccion de pantalla), y = 1 para ver distancias,
//             z = rayos usados / ancho de la textura, w = probes usadas / alto.

in vec2 vUV;

out vec4 FragColor;

layout(binding = 0) uniform sampler2D uRays;

layout(std140, binding = 4) uniform PassParams {
    vec4 uParams;
};

void main() {
    if (vUV.x > uParams.x || vUV.y > uParams.x) discard;

    vec4 c = texture(uRays, vUV / uParams.x * uParams.zw);

    if (uParams.y < 0.5) {
        FragColor = vec4(c.rgb, 1.0);
    } else if (c.a < 0.0) {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);   // cara trasera
    } else if (c.a >= 5.0e3) {
        FragColor = vec4(0.0, 0.0, 1.0, 1.0);   // cielo
    } else {
        FragColor = vec4(vec3(clamp(c.a / 50.0, 0.0, 1.0)), 1.0);
    }
}
