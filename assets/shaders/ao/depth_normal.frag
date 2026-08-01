#version 450 core

// Prepass del AO: normal view-space y profundidad LINEAL en un solo RGBA16F.
//
// La profundidad va en el alfa y no en una textura de depth aparte porque asi
// el bucle de GTAO y el blur bilateral leen UNA sola textura -- y de paso el
// motor se ahorra convertir el depth attachment de renderbuffer a textura.

in vec3 vViewNormal;
in vec3 vViewPos;

out vec4 FragColor;

void main() {
    vec3 n = normalize(vViewNormal);

    // Con CullMode::None (las salas inward=1 son cascaras de espesor cero y lo
    // exigen) tambien se rasterizan las caras de atras. Una normal apuntando en
    // contra de la camara vuelve negativo el dot(N,V) del que cuelga toda la
    // construccion del plano del corte: el AO saldria INVERTIDO justo en las
    // paredes de Cornell.
    if (!gl_FrontFacing) n = -n;

    // -z porque en la convencion de GL la camara mira hacia -z: guardarlo
    // positivo hace que "mas grande = mas lejos" y evita signos aguas abajo.
    // El fondo queda en 0 por el clear, y viewZ == 0 es el centinela de "aca no
    // hay geometria" (inalcanzable para un fragmento real: el near plane es > 0).
    FragColor = vec4(n, -vViewPos.z);
}
