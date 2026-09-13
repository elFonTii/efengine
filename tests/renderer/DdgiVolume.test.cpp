// La grilla de probes es datos puros: se testea sin GPU. Los errores de
// indexado y de layout de atlas son invisibles a ojo -- se manifiestan como
// "la GI se ve rara" sin decir donde -- asi que este es el unico lugar donde
// se pueden agarrar con precision.
#include <doctest/doctest.h>
#include <efengine/renderer/DdgiVolume.h>

#include <set>
#include <utility>
#include <vector>

using namespace efengine;
using namespace efengine::renderer;

TEST_CASE("DdgiVolume: ProbeCount es el producto de los tres ejes") {
    DdgiGrid g;
    g.counts = glm::ivec3(8, 4, 8);
    CHECK(ProbeCount(g) == 256u);

    g.counts = glm::ivec3(1, 1, 1);
    CHECK(ProbeCount(g) == 1u);

    g.counts = glm::ivec3(3, 5, 7);
    CHECK(ProbeCount(g) == 105u);
}

TEST_CASE("DdgiVolume: ProbeIndex y ProbeCoords son inversas para toda la grilla") {
    DdgiGrid g;
    g.counts = glm::ivec3(3, 5, 7);   // no cuadrada a proposito

    for (u32 i = 0u; i < ProbeCount(g); ++i) {
        const glm::ivec3 c = ProbeCoords(g, i);
        CHECK(c.x >= 0); CHECK(c.x < g.counts.x);
        CHECK(c.y >= 0); CHECK(c.y < g.counts.y);
        CHECK(c.z >= 0); CHECK(c.z < g.counts.z);
        CHECK(ProbeIndex(g, c) == i);
    }
}

TEST_CASE("DdgiVolume: el indice 0 es la esquina de origen y el ultimo la opuesta") {
    DdgiGrid g;
    g.origin  = glm::vec3(-6.0f, 0.2f, -6.0f);
    g.spacing = glm::vec3(1.5f);
    g.counts  = glm::ivec3(8, 4, 8);

    const glm::vec3 p0 = ProbeWorldPosition(g, 0u);
    CHECK(p0.x == doctest::Approx(-6.0f));
    CHECK(p0.y == doctest::Approx(0.2f));
    CHECK(p0.z == doctest::Approx(-6.0f));

    const glm::vec3 pN = ProbeWorldPosition(g, ProbeCount(g) - 1u);
    CHECK(pN.x == doctest::Approx(-6.0f + 7.0f * 1.5f));
    CHECK(pN.y == doctest::Approx( 0.2f + 3.0f * 1.5f));
    CHECK(pN.z == doctest::Approx(-6.0f + 7.0f * 1.5f));
}

TEST_CASE("DdgiVolume: ProbeWorldPosition avanza en X primero") {
    // El indice 1 tiene que ser el vecino en +X, no en +Y ni +Z: eso es lo que
    // dice probeIndex = x + countX * (...). Si esto se invierte, la GI queda
    // transpuesta y el sintoma es dificil de leer.
    DdgiGrid g;
    g.origin  = glm::vec3(0.0f);
    g.spacing = glm::vec3(2.0f, 3.0f, 4.0f);
    g.counts  = glm::ivec3(8, 4, 8);

    const glm::vec3 p1 = ProbeWorldPosition(g, 1u);
    CHECK(p1.x == doctest::Approx(2.0f));
    CHECK(p1.y == doctest::Approx(0.0f));
    CHECK(p1.z == doctest::Approx(0.0f));

    // El vecino en +Y esta countX adelante.
    const glm::vec3 py = ProbeWorldPosition(g, 8u);
    CHECK(py.x == doctest::Approx(0.0f));
    CHECK(py.y == doctest::Approx(3.0f));

    // El vecino en +Z esta countX*countY adelante.
    const glm::vec3 pz = ProbeWorldPosition(g, 32u);
    CHECK(pz.x == doctest::Approx(0.0f));
    CHECK(pz.y == doctest::Approx(0.0f));
    CHECK(pz.z == doctest::Approx(4.0f));
}

TEST_CASE("DdgiVolume: ningun tile de atlas se solapa ni se sale") {
    // Grilla no cuadrada: es donde un layout mal pensado se rompe.
    DdgiGrid g;
    g.counts = glm::ivec3(3, 5, 7);

    const glm::ivec2 tiles = AtlasTileCount(g);
    CHECK(tiles.x == 15);   // countX * countY
    CHECK(tiles.y ==  7);   // countZ

    std::set<std::pair<i32, i32>> ocupados;
    for (u32 i = 0u; i < ProbeCount(g); ++i) {
        const glm::ivec2 t = AtlasTileCoords(g, i);
        CHECK(t.x >= 0); CHECK(t.x < tiles.x);
        CHECK(t.y >= 0); CHECK(t.y < tiles.y);

        const auto [it, nuevo] = ocupados.insert({ t.x, t.y });
        CHECK(nuevo);   // dos probes en el mismo tile = irradiancia pisada
    }
    CHECK(ocupados.size() == ProbeCount(g));
}

TEST_CASE("DdgiVolume: los tamanos de atlas cuentan el borde de cada tile") {
    DdgiGrid g;
    g.counts = glm::ivec3(8, 4, 8);

    // 32 columnas x 8 filas de tiles.
    const glm::ivec2 irr = IrradianceAtlasSize(g);
    CHECK(irr.x == 32 * 10);   // kIrradianceTileBordered
    CHECK(irr.y ==  8 * 10);

    const glm::ivec2 dist = DistanceAtlasSize(g);
    CHECK(dist.x == 32 * 18);  // kDistanceTileBordered
    CHECK(dist.y ==  8 * 18);
}

TEST_CASE("DdgiVolume: SanitizeGrid clampea counts en cero y negativos") {
    DdgiGrid g;
    g.counts = glm::ivec3(0, -3, 8);
    const DdgiGrid s = SanitizeGrid(g);
    CHECK(s.counts.x == 1);
    CHECK(s.counts.y == 1);
    CHECK(s.counts.z == 8);
}

TEST_CASE("DdgiVolume: SanitizeGrid clampea counts absurdamente grandes") {
    DdgiGrid g;
    g.counts = glm::ivec3(9999, 8, 8);
    CHECK(SanitizeGrid(g).counts.x == kMaxProbesPerAxis);
}

TEST_CASE("DdgiVolume: SanitizeGrid no deja spacing en cero") {
    // Spacing cero hace que (worldPos - origin) / spacing sea inf en el shader.
    DdgiGrid g;
    g.spacing = glm::vec3(0.0f, -1.0f, 1.5f);
    const DdgiGrid s = SanitizeGrid(g);
    CHECK(s.spacing.x > 0.0f);
    CHECK(s.spacing.y > 0.0f);
    CHECK(s.spacing.z == doctest::Approx(1.5f));
}

TEST_CASE("NextRange: avanza el cursor y respeta perFrame") {
    const UpdateRange r = NextRange(0u, 8u, 256u);
    CHECK(r.first      == 0u);
    CHECK(r.count      == 8u);
    CHECK(r.nextCursor == 8u);

    const UpdateRange r2 = NextRange(r.nextCursor, 8u, 256u);
    CHECK(r2.first      ==  8u);
    CHECK(r2.count      ==  8u);
    CHECK(r2.nextCursor == 16u);
}

TEST_CASE("NextRange: el cursor envuelve al llegar al total") {
    const UpdateRange r = NextRange(250u, 8u, 256u);
    CHECK(r.first      == 250u);
    CHECK(r.count      ==   8u);   // el rango envuelve: 250..255, 0..1
    CHECK(r.nextCursor ==   2u);
}

TEST_CASE("NextRange: perFrame mayor que total no repite probes") {
    const UpdateRange r = NextRange(0u, 64u, 10u);
    CHECK(r.count == 10u);          // recortado al total, no 64
    CHECK(r.nextCursor == 0u);
}

TEST_CASE("NextRange: perFrame igual al total hace un barrido por frame") {
    const UpdateRange r = NextRange(0u, 10u, 10u);
    CHECK(r.first      ==  0u);
    CHECK(r.count      == 10u);
    CHECK(r.nextCursor ==  0u);
}

TEST_CASE("NextRange: un barrido completo visita cada probe exactamente una vez") {
    // El bug que esto agarra: un probe que nunca se actualiza y queda con la
    // irradiancia del primer frame para siempre.
    const u32 total = 100u, perFrame = 7u;
    std::vector<u32> visitas(total, 0u);

    u32 cursor = 0u;
    // 15 frames * 7 = 105 >= 100: alcanza para un barrido completo.
    for (u32 frame = 0u; frame < 15u; ++frame) {
        const UpdateRange r = NextRange(cursor, perFrame, total);
        for (u32 s = 0u; s < r.count; ++s) visitas[(r.first + s) % total] += 1u;
        cursor = r.nextCursor;
    }

    for (u32 i = 0u; i < total; ++i) {
        CHECK(visitas[i] >= 1u);   // ninguno se salteo
    }
}

TEST_CASE("NextRange: total cero no divide por cero") {
    const UpdateRange r = NextRange(0u, 8u, 0u);
    CHECK(r.count      == 0u);
    CHECK(r.nextCursor == 0u);
}

TEST_CASE("NextRange: perFrame cero no captura nada y no mueve el cursor") {
    const UpdateRange r = NextRange(5u, 0u, 256u);
    CHECK(r.count      == 0u);
    CHECK(r.nextCursor == 5u);
}

// -- Tiles del target de captura ---------------------------------------------
// El rectangulo que devuelve CaptureTileRect es lo que manda la geometria de
// cada instancia a SU celda del atlas. Un signo cambiado espeja el tile o lo
// deposita sobre el vecino, y eso NO se ve como un error: se ve como un probe
// que integro luz de otro lado. De ahi que se testee la geometria de indices y
// no solo que "devuelva algo".

namespace {
    // Aplica el rectangulo a una coordenada de clip, igual que DdgiTileClip en
    // el shader. w = 1 porque lo que se verifica es el mapeo del cubo canonico.
    glm::vec2 AlAtlas(const glm::vec4& rect, glm::vec2 clipXY) {
        return clipXY * glm::vec2(rect.x, rect.y) + glm::vec2(rect.z, rect.w);
    }

    // Bordes en NDC de la celda (face, slot), calculados aparte de la funcion
    // bajo test: si los dos usaran la misma formula el test no probaria nada.
    glm::vec4 BordesEsperados(u32 face, u32 slot) {
        const f32 ancho = 6.0f * f32(renderer::kProbeFaceSize);
        const f32 alto  = f32(renderer::kMaxProbesPerFrame) * f32(renderer::kProbeFaceSize);
        const f32 lado  = f32(renderer::kProbeFaceSize);
        return glm::vec4(2.0f * (f32(face) * lado)        / ancho - 1.0f,   // x0
                         2.0f * (f32(face) * lado + lado) / ancho - 1.0f,   // x1
                         2.0f * (f32(slot) * lado)        / alto  - 1.0f,   // y0
                         2.0f * (f32(slot) * lado + lado) / alto  - 1.0f);  // y1
    }
}

TEST_CASE("CaptureTileRect: el cubo canonico cae exactamente sobre su celda") {
    for (u32 face = 0u; face < 6u; ++face) {
        for (u32 slot : { 0u, 1u, 13u, renderer::kMaxProbesPerFrame - 1u }) {
            const glm::vec4 rect = renderer::CaptureTileRect(face, slot);
            const glm::vec4 b    = BordesEsperados(face, slot);

            // Las cuatro esquinas del volumen de vista van a las cuatro esquinas
            // de la celda, sin espejar: (-1,-1) al minimo y (+1,+1) al maximo.
            const glm::vec2 min = AlAtlas(rect, glm::vec2(-1.0f, -1.0f));
            const glm::vec2 max = AlAtlas(rect, glm::vec2( 1.0f,  1.0f));

            CHECK(min.x == doctest::Approx(b.x));
            CHECK(max.x == doctest::Approx(b.y));
            CHECK(min.y == doctest::Approx(b.z));
            CHECK(max.y == doctest::Approx(b.w));

            // El centro de la vista al centro de la celda.
            const glm::vec2 centro = AlAtlas(rect, glm::vec2(0.0f));
            CHECK(centro.x == doctest::Approx((b.x + b.y) * 0.5f));
            CHECK(centro.y == doctest::Approx((b.z + b.w) * 0.5f));
        }
    }
}

TEST_CASE("CaptureTileRect: las celdas cubren el atlas sin solaparse") {
    // Las seis caras de un slot tienen que quedar pegadas y llenar el ancho
    // entero: un hueco deja texels con el color del clear -- que en el alfa es
    // "muy lejos" -- y la integral del blend los cuenta como si fueran cielo.
    f32 anterior = -1.0f;
    for (u32 face = 0u; face < 6u; ++face) {
        const glm::vec4 rect = renderer::CaptureTileRect(face, 0u);
        const f32 x0 = AlAtlas(rect, glm::vec2(-1.0f, 0.0f)).x;
        const f32 x1 = AlAtlas(rect, glm::vec2( 1.0f, 0.0f)).x;

        CHECK(x0 == doctest::Approx(anterior));   // pegada a la anterior
        CHECK(x1 > x0);
        anterior = x1;
    }
    CHECK(anterior == doctest::Approx(1.0f));     // la ultima llega al borde

    // Lo mismo por slots en el eje vertical.
    f32 anteriorY = -1.0f;
    for (u32 slot = 0u; slot < renderer::kMaxProbesPerFrame; ++slot) {
        const glm::vec4 rect = renderer::CaptureTileRect(0u, slot);
        const f32 y0 = AlAtlas(rect, glm::vec2(0.0f, -1.0f)).y;
        const f32 y1 = AlAtlas(rect, glm::vec2(0.0f,  1.0f)).y;

        CHECK(y0 == doctest::Approx(anteriorY));
        anteriorY = y1;
    }
    CHECK(anteriorY == doctest::Approx(1.0f));
}

TEST_CASE("CaptureTileCount: una vista por cara de cada probe") {
    CHECK(renderer::CaptureTileCount(0u)  == 0u);
    CHECK(renderer::CaptureTileCount(1u)  == 6u);
    CHECK(renderer::CaptureTileCount(14u) == 84u);

    // El SSBO se aloca a este maximo: si el conteo se pasara, el draw leeria
    // fuera del buffer.
    CHECK(renderer::CaptureTileCount(renderer::kMaxProbesPerFrame)
          <= renderer::CaptureTileCount(renderer::kMaxProbesPerFrame));
}
