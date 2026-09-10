#version 450 core

// Prepass del AO: normal view-space y profundidad LINEAL en un solo RGBA16F.
//
// La profundidad va en el alfa y no en una textura de depth aparte porque asi
// el bucle de GTAO y el blur bilateral leen UNA sola textura -- y de paso el
// motor se ahorra convertir el depth attachment de renderbuffer a textura.

in vec3 vViewNormal;
in vec3 vViewPos;
in vec2 vUV;

out vec4 FragColor;

// -- Recorte por opacidad -----------------------------------------------------
// El bloque de material (binding 3) y la unidad 6 los sube Renderer::Submit en
// cualquier pase, tambien en este: no hace falta nada nuevo del lado de C++.
//
// Existe porque este pase ES el depth prepass del forward. pbr.frag descarta los
// fragmentos con alpha < alphaCutoff; si aca no se descartaran los mismos, el
// prepass dejaria profundidad donde el forward no va a pintar nada, y con
// GL_EQUAL eso tapa lo que hubiera detras: un recorte de follaje se volveria un
// agujero opaco con forma de quad.
//
// De paso arregla el AO, que hasta ahora ocluia con el quad entero en vez de con
// la silueta recortada.
layout(std140, binding = 3) uniform MaterialParams {
    vec4  uAlbedoTint;
    vec4  uEmissiveTint;
    vec4  uScalars0;
    vec4  uScalars1;      // x = alphaCutoff
    uvec4 uMapMask;
    vec4  uUvTransform;   // xy = tiling, zw = offset
};
layout(binding = 6) uniform sampler2D uOpacityMap;

const uint SLOT_OPACITY = 6u;

void main() {
    // El unico caso en que la cobertura de este pase y la del forward pueden
    // separarse es un material con mapa de altura Y de opacidad a la vez:
    // pbr.frag desplaza la UV con parallax antes de leer la opacidad y aca no.
    // Es un pixel de diferencia en el borde del recorte y no vale el coste de
    // meter POM en el prepass.
    vec2 uv = vUV * uUvTransform.xy + uUvTransform.zw;
    if ((uMapMask.x & (1u << SLOT_OPACITY)) != 0u
        && texture(uOpacityMap, uv).r < uScalars1.x) discard;

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
