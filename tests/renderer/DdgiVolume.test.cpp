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

TEST_CASE("NextRange: el ultimo lote se corta en el final de la grilla") {
    // Si en vez de cortarse envolviera (250..255, 0..1), los centros del lote
    // irian de la ultima esquina a la primera y BatchBounds devolveria la
    // grilla entera: ese frame volveria a dibujar toda la escena.
    const UpdateRange r = NextRange(250u, 8u, 256u);
    CHECK(r.first      == 250u);
    CHECK(r.count      ==   6u);
    CHECK(r.nextCursor ==   0u);
}

TEST_CASE("NextRange: ningun rango cruza el final de la grilla") {
    const u32 total = 100u, perFrame = 7u;

    u32 cursor = 0u;
    for (u32 frame = 0u; frame < 50u; ++frame) {
        const UpdateRange r = NextRange(cursor, perFrame, total);
        CHECK(r.first + r.count <= total);
        CHECK(r.count > 0u);
        cursor = r.nextCursor;
    }
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
    u32 frames = 0u;
    do {
        const UpdateRange r = NextRange(cursor, perFrame, total);
        REQUIRE(r.first + r.count <= total);
        for (u32 s = 0u; s < r.count; ++s) visitas[r.first + s] += 1u;
        cursor = r.nextCursor;
        ++frames;
    } while (cursor != 0u && frames < 100u);

    for (u32 i = 0u; i < total; ++i) {
        CHECK(visitas[i] == 1u);   // ni salteado ni repetido
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

TEST_CASE("DdgiVolume: ProbeDataBytes es un vec4 por probe") {
    DdgiGrid g;
    g.counts = glm::ivec3(10, 16, 18);
    CHECK(ProbeDataBytes(g) == 46080u);

    g.counts = glm::ivec3(0, 4, 8);
    CHECK(ProbeDataBytes(g) == 0u);
}

TEST_CASE("FibonacciDirection: direcciones unitarias y sin sesgo de hemisferio") {
    for (u32 n : { 64u, 128u, 256u }) {
        glm::vec3 suma(0.0f);
        for (u32 i = 0u; i < n; ++i) {
            const glm::vec3 d = FibonacciDirection(i, n);
            CHECK(glm::length(d) == doctest::Approx(1.0f).epsilon(1e-5));
            suma += d;
        }
        CHECK(glm::length(suma) / static_cast<f32>(n) < 0.02f);
    }
}

TEST_CASE("FibonacciDirection: la espiral va de +z a -z") {
    CHECK(FibonacciDirection(0u, 256u).z   >  0.99f);
    CHECK(FibonacciDirection(255u, 256u).z < -0.99f);
}

TEST_CASE("RandomRayRotation: ortonormal y sin reflexion") {
    for (u32 seed : { 0u, 1u, 7u, 123456u, 0xFFFFFFFFu }) {
        const glm::mat3 r = RandomRayRotation(seed);
        const glm::mat3 i = glm::transpose(r) * r;
        for (int c = 0; c < 3; ++c) {
            for (int f = 0; f < 3; ++f) {
                CHECK(i[c][f] == doctest::Approx(c == f ? 1.0f : 0.0f).epsilon(1e-5));
            }
        }
        CHECK(glm::determinant(r) == doctest::Approx(1.0f).epsilon(1e-5));
    }
}

TEST_CASE("RandomRayRotation: determinista por semilla") {
    CHECK(RandomRayRotation(42u) == RandomRayRotation(42u));
    const glm::vec3 z(0.0f, 0.0f, 1.0f);
    CHECK(glm::length(RandomRayRotation(42u) * z - RandomRayRotation(43u) * z) > 1e-3f);
}

TEST_CASE("RandomRayRotation: sobre muchas semillas no favorece ninguna direccion") {
    glm::vec3 suma(0.0f);
    for (u32 s = 0u; s < 4096u; ++s) suma += RandomRayRotation(s) * glm::vec3(0.0f, 0.0f, 1.0f);
    CHECK(glm::length(suma) / 4096.0f < 0.05f);
}

TEST_CASE("DistanceClamp: 1,5 veces la diagonal de la celda") {
    DdgiGrid g;
    g.spacing = glm::vec3(1.0f, 2.0f, 2.0f);
    CHECK(DistanceClamp(g) == doctest::Approx(4.5f));
}

TEST_CASE("DistanceClamp: un spacing en cero no da cero") {
    DdgiGrid g;
    g.spacing = glm::vec3(0.0f);
    CHECK(DistanceClamp(g) > 0.0f);
}

TEST_CASE("EffectiveHysteresis: las dos primeras escrituras sobreescriben") {
    CHECK(EffectiveHysteresis(0u, 0.97f) == 0.0f);
    CHECK(EffectiveHysteresis(1u, 0.97f) == 0.0f);
}

TEST_CASE("EffectiveHysteresis: promedio progresivo hasta el tope") {
    CHECK(EffectiveHysteresis(2u, 0.97f)    == doctest::Approx(0.5f));
    CHECK(EffectiveHysteresis(3u, 0.97f)    == doctest::Approx(2.0f / 3.0f));
    CHECK(EffectiveHysteresis(4u, 0.97f)    == doctest::Approx(0.75f));
    CHECK(EffectiveHysteresis(255u, 0.97f)  == doctest::Approx(0.97f));
    CHECK(EffectiveHysteresis(1000u, 0.97f) == doctest::Approx(0.97f));
}

TEST_CASE("EffectiveHysteresis: hMax fuera de [0,1] se clampea") {
    CHECK(EffectiveHysteresis(100u, 2.0f)  == doctest::Approx(0.99f));
    CHECK(EffectiveHysteresis(100u, -1.0f) == 0.0f);
}

TEST_CASE("DdgiVolume: ProbeStateBytes es un uvec4 por probe") {
    DdgiGrid g;
    g.counts = glm::ivec3(2, 3, 4);
    CHECK(ProbeStateBytes(g) == 24u * 16u);
}

TEST_CASE("DdgiVolume: el buffer de rayos entra en 1 MB de RGBA16F") {
    CHECK(kMaxProbesPerFrame == 512u);
    CHECK(static_cast<usize>(kMaxRaysPerProbe) * kMaxProbesPerFrame * 8u <= 1024u * 1024u);
}
