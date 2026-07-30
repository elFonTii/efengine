#include "efengine/renderer/BoxMesh.h"

#include <efengine/renderer/Mesh.h>

#include <algorithm>
#include <utility>

namespace efengine {
namespace renderer {

    namespace {

        // glm::vec3 no se puede indexar con un int en contexto constexpr, pero si
        // en runtime: este helper existe para que la tabla de abajo pueda guardar
        // "que componente de half es mi eje U" como un indice.
        f32 halfDe(const glm::vec3& half, int axis) { return half[axis]; }

        // Base ortonormal de cada cara: normal saliente 'n' y dos ejes en el plano
        // 'u' y 'v' con cross(u, v) == n. Que la base sea derecha es lo que hace
        // que un quad en orden (u0,v0) (u1,v0) (u1,v1) (u0,v1) quede CCW visto
        // desde +n, y por lo tanto que el winding coincida con la normal.
        struct FaceBasis {
            const char* name;
            glm::vec3   n, u, v;
            int         axisU, axisV, axisN;   // que componente de 'half' es cada uno
        };

        const FaceBasis kFaces[kBoxFaceCount] = {
            { "pared_xneg", {-1.0f, 0.0f, 0.0f}, { 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f,  0.0f}, 2, 1, 0 },
            { "pared_xpos", { 1.0f, 0.0f, 0.0f}, { 0.0f, 0.0f,-1.0f}, {0.0f, 1.0f,  0.0f}, 2, 1, 0 },
            { "piso",       { 0.0f,-1.0f, 0.0f}, { 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f,  1.0f}, 0, 2, 1 },
            { "techo",      { 0.0f, 1.0f, 0.0f}, { 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 0, 2, 1 },
            { "pared_zneg", { 0.0f, 0.0f,-1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f,  0.0f}, 0, 1, 2 },
            { "pared_zpos", { 0.0f, 0.0f, 1.0f}, { 1.0f, 0.0f, 0.0f}, {0.0f, 1.0f,  0.0f}, 0, 1, 2 },
        };

        // Un quad de la cara, en coords locales (u, v) de esa cara.
        void emitirQuad(const FaceBasis& f, const glm::vec3& half, bool inward, f32 uvPerMeter,
                        f32 u0, f32 v0, f32 u1, f32 v1, BoxFace& out) {
            // Un strip degenerado no aporta triangulos y solo mete area cero.
            if (u1 - u0 <= 1e-5f || v1 - v0 <= 1e-5f) return;

            const glm::vec3 base   = f.n * half[f.axisN];
            const glm::vec3 normal = inward ? -f.n : f.n;
            const u32       first  = static_cast<u32>(out.vertices.size());

            const f32 us[4] = { u0, u1, u1, u0 };
            const f32 vs[4] = { v0, v0, v1, v1 };

            for (int i = 0; i < 4; ++i) {
                Vertex vert;
                vert.position = base + f.u * us[i] + f.v * vs[i];
                vert.normal   = normal;
                vert.uv       = glm::vec2(us[i], vs[i]) * uvPerMeter;
                // Tangente sobre +U del UV. Con inward el sistema queda zurdo,
                // que no importa mientras estas cajas no usen normal map.
                vert.tangent  = f.u;
                out.vertices.push_back(vert);
            }

            // Saliente: CCW desde +n. Adentro: los dos triangulos al reves.
            if (inward) {
                const u32 idx[6] = { 0u, 2u, 1u, 0u, 3u, 2u };
                for (u32 i : idx) out.indices.push_back(first + i);
            } else {
                const u32 idx[6] = { 0u, 1u, 2u, 0u, 2u, 3u };
                for (u32 i : idx) out.indices.push_back(first + i);
            }
        }

    } // namespace

    std::vector<BoxFace> BoxMeshData(const BoxParams& p) {
        const bool inward = p.inward != 0u;

        std::vector<BoxFace> caras;
        caras.reserve(kBoxFaceCount);

        for (u32 i = 0u; i < kBoxFaceCount; ++i) {
            const FaceBasis& f = kFaces[i];

            BoxFace cara;
            cara.name = f.name;

            const f32 halfU = halfDe(p.half, f.axisU);
            const f32 halfV = halfDe(p.half, f.axisV);

            if (i != p.holeFace) {
                emitirQuad(f, p.half, inward, p.uvPerMeter, -halfU, -halfV, halfU, halfV, cara);
            } else {
                // Marco de 4 quads alrededor del hueco, clampeado a la cara.
                const f32 u0 = glm::clamp(glm::min(p.hole.x, p.hole.z), -halfU, halfU);
                const f32 v0 = glm::clamp(glm::min(p.hole.y, p.hole.w), -halfV, halfV);
                const f32 u1 = glm::clamp(glm::max(p.hole.x, p.hole.z), -halfU, halfU);
                const f32 v1 = glm::clamp(glm::max(p.hole.y, p.hole.w), -halfV, halfV);

                emitirQuad(f, p.half, inward, p.uvPerMeter, -halfU, -halfV, halfU, v0,    cara);  // abajo
                emitirQuad(f, p.half, inward, p.uvPerMeter, -halfU,  v1,    halfU, halfV, cara);  // arriba
                emitirQuad(f, p.half, inward, p.uvPerMeter, -halfU,  v0,    u0,    v1,    cara);  // izquierda
                emitirQuad(f, p.half, inward, p.uvPerMeter,  u1,     v0,    halfU, v1,    cara);  // derecha
            }

            caras.push_back(std::move(cara));
        }

        return caras;
    }

    Model MakeBoxModel(const BoxParams& p) {
        std::vector<Mesh> meshes;
        for (BoxFace& cara : BoxMeshData(p)) {
            // Una cara enteramente abierta no tiene triangulos: subirla crearia un
            // VAO vacio que el pase dibujaria con 0 indices.
            if (cara.indices.empty()) continue;
            meshes.emplace_back(cara.vertices, cara.indices, cara.name);
        }
        return Model(std::move(meshes));
    }

}
}
