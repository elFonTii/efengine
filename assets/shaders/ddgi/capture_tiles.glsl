// assets/shaders/ddgi/capture_tiles.glsl
// EL contrato de la captura instanciada de probes.
//
// -- El problema --
// La captura necesita 6 vistas por probe (las caras del cubo) y actualiza varios
// probes por frame. Emitido como un draw por (probe, cara, objeto) eso son
// probes * 6 * objetos draws: con 14 probes y 6 objetos mas el cielo, 588 draws
// por frame para una escena de SEIS. La GPU los despachaba en 0.150 ms; la CPU
// tardaba 2.270 ms en emitirlos. Un ratio de 15:1 no es trabajo, es overhead de
// submission puro -- y escala con probes*caras*objetos, asi que con 200 objetos
// serian ~19.000 draws y el motor se hunde por CPU antes de que la GPU se entere.
//
// -- La solucion --
// El target de captura NO es un cubemap: es un atlas 2D donde cada (cara, slot)
// es un rectangulo. Eso permite dibujar TODAS las vistas de TODOS los probes del
// frame en un solo draw instanciado por objeto: la instancia elige su vista de
// este buffer, se transforma a su rectangulo del atlas con una escala y un
// offset en clip space, y se recorta a el con gl_ClipDistance.
//
// probes*6 draws por objeto pasan a UNO. Con la escena de prueba: 588 -> 7.
//
// -- Por que gl_ClipDistance y no un viewport por instancia --
// gl_ViewportIndex existe pero GL garantiza apenas 16 viewports, y aca hacen
// falta probes*6 (84 con la configuracion por defecto). Los planos de recorte
// son 8 garantizados y hacen falta 4 -- uno por lado del rectangulo --, asi que
// el limite no depende de cuantos probes se actualicen por frame.
//
// Sin el recorte, un triangulo que se sale de su vista aterriza dentro del atlas
// igual (la escala y el offset lo metieron ahi) y pinta encima del tile vecino:
// la radiancia de una cara se filtra en otra y el probe integra luz que nunca
// vio. El recorte de GL contra el cubo NDC no alcanza, porque despues de la
// escala el cubo NDC es el ATLAS ENTERO, no el tile.

struct DdgiCaptureTile {
    mat4 viewProj;         // vista de esta (probe, cara)
    mat4 invViewProjRot;   // su inversa sin traslacion, para el cielo
    vec4 rect;             // xy = escala en NDC, zw = offset en NDC
    vec4 probeCenter;      // .xyz = centro del probe en mundo
};

// Binding 0 de SSBO. Lo llena DdgiPass una vez por frame.
layout(std430, binding = 0) readonly buffer DdgiCaptureTiles {
    DdgiCaptureTile uTiles[];
};

// Lleva una posicion de clip de la vista de la instancia al rectangulo que le
// toca en el atlas. El * clip.w del offset es lo que hace que el corrimiento
// sobreviva a la division por perspectiva: sin el, el tile se desplazaria segun
// la profundidad de cada vertice.
vec4 DdgiTileClip(vec4 clip, vec4 rect) {
    return vec4(clip.xy * rect.xy + rect.zw * clip.w, clip.zw);
}

// Los cuatro planos que recortan la instancia a su vista. Se evaluan sobre la
// posicion de clip ORIGINAL (antes de la escala al tile), donde el volumen de
// vista es el cubo canonico y los planos son simplemente |x| <= w, |y| <= w.
// Positivo = adentro.
//
// Todo vertex shader que dibuje mientras los planos esten habilitados TIENE que
// escribir estas cuatro distancias: un shader que no las escribe recorta contra
// basura, y el resultado es indefinido, no "sin recortar".
void DdgiWriteTileClip(vec4 clip, out float d0, out float d1, out float d2, out float d3) {
    d0 = clip.x + clip.w;   // x >= -w
    d1 = clip.w - clip.x;   // x <=  w
    d2 = clip.y + clip.w;   // y >= -w
    d3 = clip.w - clip.y;   // y <=  w
}
