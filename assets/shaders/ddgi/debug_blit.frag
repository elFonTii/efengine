#version 450 core
// Vuelca el target de captura de probes en una esquina de la pantalla.
// Es el instrumento que hace verificable la captura antes de que exista ningun
// blend: si los tiles de cara aparecen en el orden equivocado, girados o
// espejados, se ve aca y no despues como "la GI esta rara".
//
// PassParams: x = escala del recuadro (fraccion del ancho de pantalla),
//             y = 1 para dividir por el alfa/1e4 y ver la distancia en vez del color.

in vec2 vUV;

out vec4 FragColor;

layout(binding = 0) uniform sampler2D uCapture;

layout(std140, binding = 4) uniform PassParams {
    vec4 uParams;
};

void main() {
    // El recuadro ocupa la esquina inferior izquierda; afuera se descarta para
    // no pisar la escena. El target es 96x512, asi que el alto del recuadro es
    // el ancho por esa relacion de aspecto.
    vec2 box = vec2(uParams.x, uParams.x * 512.0 / 96.0);
    if (vUV.x > box.x || vUV.y > box.y) discard;

    vec2 uv = vUV / box;
    vec4 c  = texture(uCapture, uv);

    // La distancia se normaliza contra el clear de 1e4 para que entre en [0,1].
    FragColor = (uParams.y > 0.5) ? vec4(vec3(c.a / 1e4), 1.0) : vec4(c.rgb, 1.0);
}
