#include <doctest/doctest.h>
#include <efengine/serialization/SceneDocument.h>
#include <efengine/serialization/ComponentPayloads.h>
#include <utility>
#include <vector>

using namespace efengine;
using namespace efengine::serialization;

namespace {
    template <class Payload>
    ComponentRecord componente(Payload p, const char* typeName, StringTable& strings) {
        BinaryWriter w;
        SerializePayload(w, p);

        ComponentRecord rec;
        rec.typeNameStr = strings.Intern(typeName);
        rec.payload     = w.Take();
        return rec;
    }

    // Documento chico pero con las cuatro formas de nodo, para que el archivo tenga
    // strings, materiales, bindings, payloads y luces que se puedan cortar por el medio.
    std::vector<u8> bytesValidos() {
        SceneDocument d;
        d.iblIntensity = 0.2f;

        const u32 sRoot = d.strings.Intern("root");
        const u32 sMalla = d.strings.Intern("assets/models/x.fbx");
        const u32 sSub  = d.strings.Intern("submesh");
        const u32 sMat  = d.strings.Intern("mat");
        const u32 sBeh  = d.strings.Intern("Girar");
        const u32 sLuz  = d.strings.Intern("luz");

        MaterialRecord m;
        m.nameStr = sMat;
        m.textures.push_back(TextureRef{ 0u, sMalla, 1u });
        // Valores no-default para que el sweep de truncacion/corrupcion recorra
        // tambien los bytes que agregaron v2 (20) y v4 (16) al final del record.
        m.emissiveTint      = glm::vec3(0.5f, 0.25f, 0.125f);
        m.emissiveIntensity = 8.0f;
        m.normalStrength    = 1.5f;
        m.uvTiling          = glm::vec2(6.0f, 3.0f);
        m.uvOffset          = glm::vec2(0.125f, 0.375f);
        d.materials.push_back(m);

        NodeRecord root;
        root.nameStr = sRoot;
        root.parent  = kInvalidIndex;
        d.nodes.push_back(root);

        NodeRecord conMalla;
        conMalla.nameStr = sSub;
        conMalla.parent  = 0u;
        {
            MeshPayload mp;
            mp.str = sMalla;
            mp.bindings.push_back(MaterialBinding{ sSub, 0u });
            conMalla.components.push_back(componente(std::move(mp), kMeshComponentName, d.strings));
        }
        BehaviorRecord b;
        b.typeNameStr = sBeh;
        b.payload = { 1u, 2u, 3u };
        conMalla.behaviors.push_back(b);
        d.nodes.push_back(conMalla);

        NodeRecord conLuz;
        conLuz.nameStr = sLuz;
        conLuz.parent  = 1u;
        {
            LightPayload lp;
            lp.kind = LightKindId::Directional;
            conLuz.components.push_back(componente(lp, kLightComponentName, d.strings));
        }
        d.nodes.push_back(conLuz);

        d.primarySunNode = 2u;

        std::vector<u8> out;
        const bool ok = WriteSceneDocument(d, out);
        REQUIRE(ok);
        return out;
    }
}

TEST_CASE("EfeTruncation: el archivo completo parsea (control)") {
    const std::vector<u8> bytes = bytesValidos();
    SceneDocument dst;
    CHECK(ParseSceneDocument(bytes.data(), bytes.size(), dst));
    CHECK(dst.nodes.size() == 3u);
}

TEST_CASE("EfeTruncation: cortado a CUALQUIER largo falla limpio y sin crashear") {
    const std::vector<u8> bytes = bytesValidos();
    REQUIRE(bytes.size() > 20u);

    // Cada largo posible menor al total: ninguno debe parsear, ninguno debe crashear.
    u32 parsearonDeMas = 0u;
    for (usize len = 0u; len < bytes.size(); ++len) {
        SceneDocument dst;
        if (ParseSceneDocument(bytes.data(), len, dst)) ++parsearonDeMas;
        // Sea que fallo o no, el documento nunca queda con nodos invalidos:
        for (usize i = 1u; i < dst.nodes.size(); ++i) {
            CHECK(dst.nodes[i].parent < static_cast<u32>(i));
        }
    }
    CHECK(parsearonDeMas == 0u);
}

TEST_CASE("EfeTruncation: un byte corrupto en cualquier posicion no crashea ni allocua de mas") {
    const std::vector<u8> original = bytesValidos();

    // Poner 0xFF en cada posicion, de a una. Un count corrupto es el caso peligroso:
    // BinaryReader::Count tiene que rechazarlo ANTES de dimensionar un vector.
    for (usize i = 0u; i < original.size(); ++i) {
        std::vector<u8> corrupto = original;
        corrupto[i] = 0xFFu;

        SceneDocument dst;
        // El resultado puede ser true o false (un flip puede caer en un f32 y seguir
        // siendo valido). Lo que se verifica es que no rompe y que si dice que si,
        // el invariante de pre-orden se cumple.
        if (ParseSceneDocument(corrupto.data(), corrupto.size(), dst)) {
            for (usize n = 1u; n < dst.nodes.size(); ++n) {
                CHECK(dst.nodes[n].parent < static_cast<u32>(n));
            }
            CHECK((dst.primarySunNode == kInvalidIndex
                   || dst.primarySunNode < static_cast<u32>(dst.nodes.size())));
        }
    }
}

TEST_CASE("EfeTruncation: cero corrupto en cada posicion tampoco crashea") {
    const std::vector<u8> original = bytesValidos();

    for (usize i = 0u; i < original.size(); ++i) {
        std::vector<u8> corrupto = original;
        corrupto[i] = 0x00u;

        SceneDocument dst;
        if (ParseSceneDocument(corrupto.data(), corrupto.size(), dst)) {
            for (usize n = 1u; n < dst.nodes.size(); ++n) {
                CHECK(dst.nodes[n].parent < static_cast<u32>(n));
            }
        }
    }
}

TEST_CASE("EfeTruncation: parent que apunta hacia adelante se rechaza") {
    // Se construye a mano un documento que rompe el pre-orden: el nodo 1 dice que su
    // padre es el 2, que todavia no existe cuando el resolve lo necesita.
    SceneDocument d;
    NodeRecord root; root.nameStr = d.strings.Intern("root"); root.parent = kInvalidIndex;
    NodeRecord a;    a.nameStr    = d.strings.Intern("a");    a.parent    = 2u;
    NodeRecord b;    b.nameStr    = d.strings.Intern("b");    b.parent    = 0u;
    d.nodes.push_back(root);
    d.nodes.push_back(a);
    d.nodes.push_back(b);

    std::vector<u8> bytes;
    REQUIRE(WriteSceneDocument(d, bytes));

    SceneDocument dst;
    CHECK_FALSE(ParseSceneDocument(bytes.data(), bytes.size(), dst));
    CHECK(dst.nodes.empty());
}

TEST_CASE("EfeTruncation: primarySunNode fuera de rango se rechaza") {
    SceneDocument d;
    NodeRecord root; root.nameStr = d.strings.Intern("root"); root.parent = kInvalidIndex;
    d.nodes.push_back(root);
    d.primarySunNode = 99u;

    std::vector<u8> bytes;
    REQUIRE(WriteSceneDocument(d, bytes));

    SceneDocument dst;
    CHECK_FALSE(ParseSceneDocument(bytes.data(), bytes.size(), dst));
}
