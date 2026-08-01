#version 450 core

// GTAO -- Ground Truth Ambient Occlusion (Jimenez et al., 2016).
//
// Por cada uno de los 'slices' cortes: un plano que contiene la direccion de
// vista y una direccion de pantalla. Sobre el se marcha hacia ambos lados
// buscando los angulos de horizonte h1 y h2, y el arco de visibilidad se
// resuelve con la integral cerrada del paper en vez de contar muestras
// ocluidas como el SSAO clasico. De ahi sale la ponderacion coseno correcta y,
// gratis, el bent normal: la bisectriz de cada corte.

in vec2 vUV;
out vec4 FragColor;

layout(binding = 0) uniform sampler2D uDepthNormal;   // xyz = normal view, w = viewZ lineal

layout(std140, binding = 4) uniform AoPassParams {
    mat4  uViewToWorld;
    vec4  uProjInfo;    // xy = reconstruccion view-space, zw = 1/resolucion
    vec4  uParams0;     // radius, thickness, intensity, maxScreenRadius
    vec4  uParams1;     // projScale, _, _, _
    ivec4 uCounts;      // x=slices, y=steps, z=dirBlur (sin uso aca), w=debugView
};

const float kPi = 3.14159265359;

// Distancia MINIMA de un tap al pixel central, en pixeles. Ver el descarte en
// el bucle de marcha: por debajo de esto el tap vuelve a leer el propio texel y
// el horizonte que devuelve es basura.
const float kMinTapPx = 2.0;

vec3 ViewPos(vec2 uv, float viewZ) {
    vec2 ndc = uv * 2.0 - 1.0;
    return vec3(ndc * uProjInfo.xy, -1.0) * viewZ;
}

// Interleaved gradient noise. SOLO de gl_FragCoord, SIN componente de frame: no
// hay acumulacion temporal en este pase, asi que un ruido que cambia por frame
// PRODUCE el titileo en vez de disolverlo.
float Ign(vec2 p) {
    return fract(52.9829189 * fract(dot(p, vec2(0.06711056, 0.00583715))));
}

void main() {
    vec4  centro = texture(uDepthNormal, vUV);
    float viewZ  = centro.w;

    // Las vistas 3 y 4 vuelcan el prepass: este shader es el unico que lo tiene
    // bindeado. Visibilidad 1 para no alterar la imagen antes de mostrarlas.
    if (uCounts.w == 3) { FragColor = vec4(normalize(centro.xyz), 1.0); return; }
    if (uCounts.w == 4) { FragColor = vec4(vec3(fract(viewZ)), 1.0); return; }

    // Fondo sin geometria: visibilidad plena.
    if (viewZ <= 0.0) { FragColor = vec4(0.0, 0.0, 0.0, 1.0); return; }

    vec3 P = ViewPos(vUV, viewZ);
    vec3 V = normalize(-P);
    vec3 N = normalize(centro.xyz);

    vec3 nWorld = normalize(mat3(uViewToWorld) * N);

    // El radio en METROS del panel proyectado a PIXELES, con techo: el caso
    // "camara pegada a una pared" hace explotar radius/viewZ.
    float radioPx = min(uParams0.x * uParams1.x / viewZ, uParams0.w);
    if (radioPx < 1.0) { FragColor = vec4(nWorld, 1.0); return; }

    float ruido  = Ign(gl_FragCoord.xy);
    int   slices = uCounts.x;
    int   steps  = uCounts.y;

    float visibilidad = 0.0;
    vec3  bent        = vec3(0.0);
    float pesoTotal   = 0.0;

    for (int s = 0; s < slices; ++s) {
        float phi   = (float(s) + ruido) * kPi / float(slices);
        vec2  omega = vec2(cos(phi), sin(phi));

        // Base del plano del corte.
        vec3  sliceN   = cross(vec3(omega, 0.0), V);
        float sliceLen = length(sliceN);
        if (sliceLen < 1e-5) continue;
        sliceN /= sliceLen;

        // OJO CON EL ORDEN. Este es el eje contra el que se miden los signos de
        // TODO lo que sigue: gamma, los dos horizontes y la bisectriz del bent
        // normal. Por la identidad (u x V) x V = -u, cross(sliceN, V) devuelve
        // -omega: las muestras de +omega (uvA = vUV + desplaz) caerian en el
        // semiplano NEGATIVO mientras h2 les pone signo positivo, o sea h1 y h2
        // cruzados. El error es exactamente cero cuando el corte es simetrico y
        // maximo en las aristas y en las superficies al ras.
        vec3 tangente = cross(V, sliceN);

        // N proyectada al plano del corte.
        vec3  projN    = N - sliceN * dot(N, sliceN);
        float projNLen = length(projN);
        if (projNLen < 1e-5) continue;
        vec3 projNdir = projN / projNLen;

        // Angulo de la normal proyectada respecto de la vista, con signo.
        float gamma = sign(dot(projNdir, tangente))
                    * acos(clamp(dot(projNdir, V), -1.0, 1.0));

        float cosH1 = -1.0;   // lado -omega
        float cosH2 = -1.0;   // lado +omega

        for (int i = 1; i <= steps; ++i) {
            // El jitter del ruido tambien va en el paso: sin el, los pasos caen
            // siempre en los mismos anillos y aparece banding concentrico.
            float t     = (float(i) - 0.5 + ruido) / float(steps);
            float offPx = t * radioPx;

            // Un tap a menos de un par de texeles del centro vuelve a leer el
            // PROPIO texel: la profundidad es la misma, asi que dA termina
            // siendo el corrimiento lateral puro, casi perpendicular a V, y
            // devuelve un horizonte falso de ~90 grados. Como cosH acumula con
            // max(), ese tap degenerado se queda para siempre y tapa los
            // horizontes correctos que traen los taps de mas lejos.
            //
            // Es lo que hacia que SUBIR el slider de pasos empeorara la imagen:
            // el paso mide radioPx/steps, asi que cuanto mas fino, mas taps
            // caen adentro del texel propio. Con radioPx 50 y 32 pasos el
            // primer tap cae a 0.8 px.
            if (offPx < kMinTapPx) continue;

            vec2 desplaz = omega * offPx * uProjInfo.zw;

            vec2  uvA = vUV + desplaz;
            float zA  = texture(uDepthNormal, uvA).w;
            if (zA > 0.0) {
                vec3  dA = ViewPos(uvA, zA) - P;
                float lA = length(dA);
                if (lA > 1e-4) {
                    // Heuristica de grosor: una muestra mas lejos que el radio se
                    // desvanece. Sin esto, un objeto lejano alineado en pantalla
                    // ocluye a uno cercano.
                    float peso = clamp((uParams0.x - lA) / max(uParams0.x * uParams0.y, 1e-4),
                                       0.0, 1.0);
                    cosH2 = max(cosH2, mix(-1.0, dot(dA / lA, V), peso));
                }
            }

            vec2  uvB = vUV - desplaz;
            float zB  = texture(uDepthNormal, uvB).w;
            if (zB > 0.0) {
                vec3  dB = ViewPos(uvB, zB) - P;
                float lB = length(dB);
                if (lB > 1e-4) {
                    float peso = clamp((uParams0.x - lB) / max(uParams0.x * uParams0.y, 1e-4),
                                       0.0, 1.0);
                    cosH1 = max(cosH1, mix(-1.0, dot(dB / lB, V), peso));
                }
            }
        }

        float h1 = -acos(clamp(cosH1, -1.0, 1.0));
        float h2 =  acos(clamp(cosH2, -1.0, 1.0));

        // Los horizontes se clampean al hemisferio de la normal.
        h1 = gamma + max(h1 - gamma, -0.5 * kPi);
        h2 = gamma + min(h2 - gamma,  0.5 * kPi);

        // La integral cerrada del arco de visibilidad. ESTO es lo que separa a
        // GTAO del SSAO de contar muestras: el coseno sale exacto.
        float arco = 0.25 * (-cos(2.0 * h1 - gamma) + cos(gamma) + 2.0 * h1 * sin(gamma))
                   + 0.25 * (-cos(2.0 * h2 - gamma) + cos(gamma) + 2.0 * h2 * sin(gamma));

        visibilidad += projNLen * arco;

        // Bent normal: la bisectriz del arco abierto, en la base del plano.
        float bisectriz = (h1 + h2) * 0.5;
        bent      += projNLen * (V * cos(bisectriz) + tangente * sin(bisectriz));
        pesoTotal += projNLen;
    }

    if (pesoTotal < 1e-5) { FragColor = vec4(nWorld, 1.0); return; }

    visibilidad = clamp(visibilidad / float(slices), 0.0, 1.0);
    visibilidad = pow(visibilidad, uParams0.z);

    // Correccion del sesgo hacia la vista. Sumar la bisectriz de cada corte NO
    // reconstruye N: en el caso sin oclusion cada corte aporta la normal
    // proyectada sobre SU plano, y la suma sobre los cortes da
    //
    //     Σ projN = (S/2) * (N + (N·V)V)
    //
    // o sea N contaminada con una copia de la direccion de vista. El error es
    // cero mirando de frente y cero al ras, pero llega a ~18 grados a 45, y se
    // ve como un degradado suave a lo largo de una pared PLANA -- justo donde
    // el bent normal tendria que ser constante e igual a la normal.
    //
    // La contaminacion es una transformacion lineal conocida, (I + V·Vt), asi
    // que se despeja exacto: su inversa es (I - 0.5*V·Vt).
    bent -= 0.5 * dot(bent, V) * V;

    vec3 bentView  = (dot(bent, bent) > 1e-8) ? normalize(bent) : N;
    vec3 bentWorld = normalize(mat3(uViewToWorld) * bentView);

    FragColor = vec4(bentWorld, visibilidad);
}
