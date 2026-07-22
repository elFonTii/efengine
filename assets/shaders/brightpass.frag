#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uScene;      // color HDR lineal de la escena (RGBA16F)
uniform float uThreshold;      // radiancia minima que "florece" (~1.0)
uniform float uKnee;           // [0,1] suavidad del umbral (0 = corte duro)

// Umbral con rodilla suave (Unity/Karis): transicion cuadratica alrededor del
// umbral en vez de un corte binario, para evitar flicker en los bordes.
void main() {
    vec3 hdr = texture(uScene, vUV).rgb;
    float luma = dot(hdr, vec3(0.2126, 0.7152, 0.0722));

    float knee = uThreshold * uKnee;
    float soft = clamp(luma - uThreshold + knee, 0.0, 2.0 * knee);
    soft = (soft * soft) / (4.0 * knee + 1e-4);
    float contrib = max(soft, luma - uThreshold) / max(luma, 1e-4);

    FragColor = vec4(hdr * contrib, 1.0);   // 0 debajo del umbral, sube suave arriba
}
