#version 450 core
in vec2 vUV;
out vec4 FragColor;

layout(binding = 0) uniform sampler2D uScene;      // color HDR lineal de la escena (RGBA16F)

layout(std140, binding = 4) uniform PassParams {
    vec4 uParams;   // x = threshold (radiancia minima que florece), y = knee
};

// Umbral con rodilla suave (Unity/Karis): transicion cuadratica alrededor del
// umbral en vez de un corte binario, para evitar flicker en los bordes.
void main() {
    vec3 hdr = texture(uScene, vUV).rgb;
    float luma = dot(hdr, vec3(0.2126, 0.7152, 0.0722));

    float knee = uParams.x * uParams.y;
    float soft = clamp(luma - uParams.x + knee, 0.0, 2.0 * knee);
    soft = (soft * soft) / (4.0 * knee + 1e-4);
    float contrib = max(soft, luma - uParams.x) / max(luma, 1e-4);

    FragColor = vec4(hdr * contrib, 1.0);   // 0 debajo del umbral, sube suave arriba
}
