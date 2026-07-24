#include "efengine/renderer/Primitives.h"

#include "efengine/renderer/Mesh.h"
#include "efengine/renderer/Vertex.h"

#include <glm/glm.hpp>

#include <cmath>
#include <utility>
#include <vector>

namespace efengine {
namespace renderer {
namespace primitives {

namespace {

    // Empaqueta vértices + índices en un Model de una sola malla, evitando
    // repetir el trío emplace/move en cada generador.
    Model buildModel(std::vector<Vertex> vertices, std::vector<u32> indices,
                     const std::string& materialName) {
        std::vector<Mesh> meshes;
        meshes.emplace_back(vertices, indices, materialName);
        return Model(std::move(meshes));
    }

    // Cose una rejilla de (rows x (sectors+1)) vértices en triángulos. Los
    // vértices deben estar ordenados fila por fila, cada fila con sectors+1
    // columnas (la última columna duplica la primera para cerrar las UVs).
    // Las filas polares con radio 0 generan triángulos degenerados inofensivos.
    void stitchGrid(std::vector<u32>& indices, u32 rows, u32 sectors) {
        for (u32 i = 0; i + 1 < rows; ++i) {
            for (u32 j = 0; j < sectors; ++j) {
                const u32 a = i * (sectors + 1) + j;
                const u32 b = a + (sectors + 1);
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(a + 1);
                indices.push_back(a + 1);
                indices.push_back(b);
                indices.push_back(b + 1);
            }
        }
    }

} // namespace

Model Plane(const std::string& materialName, f32 halfSize, f32 tiles) {
    const std::vector<Vertex> vertices = {
        // position                          normal              uv                tangent
        { {-halfSize, 0.0f, -halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f,  0.0f},  {1.0f, 0.0f, 0.0f} },
        { { halfSize, 0.0f, -halfSize}, {0.0f, 1.0f, 0.0f}, {tiles, 0.0f},  {1.0f, 0.0f, 0.0f} },
        { { halfSize, 0.0f,  halfSize}, {0.0f, 1.0f, 0.0f}, {tiles, tiles}, {1.0f, 0.0f, 0.0f} },
        { {-halfSize, 0.0f,  halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f,  tiles}, {1.0f, 0.0f, 0.0f} },
    };
    const std::vector<u32> indices = { 0, 1, 2, 2, 3, 0 };
    return buildModel(vertices, indices, materialName);
}

Model Cube(const std::string& materialName, f32 halfExtent) {
    const f32 h = halfExtent;

    // Cada cara: normal (n), dirección u (tangent) y dirección v (bitangent).
    struct Face { glm::vec3 n; glm::vec3 t; };
    const Face faces[6] = {
        { { 1.0f,  0.0f,  0.0f}, { 0.0f, 0.0f, -1.0f} }, // +X
        { {-1.0f,  0.0f,  0.0f}, { 0.0f, 0.0f,  1.0f} }, // -X
        { { 0.0f,  1.0f,  0.0f}, { 1.0f, 0.0f,  0.0f} }, // +Y
        { { 0.0f, -1.0f,  0.0f}, { 1.0f, 0.0f,  0.0f} }, // -Y
        { { 0.0f,  0.0f,  1.0f}, { 1.0f, 0.0f,  0.0f} }, // +Z
        { { 0.0f,  0.0f, -1.0f}, {-1.0f, 0.0f,  0.0f} }, // -Z
    };

    std::vector<Vertex> vertices;
    vertices.reserve(24);
    std::vector<u32> indices;
    indices.reserve(36);

    for (const Face& f : faces) {
        const glm::vec3 b = glm::cross(f.n, f.t); // bitangent = dirección v
        // Esquinas en el plano de la cara con UV (0,0),(1,0),(1,1),(0,1).
        const glm::vec2 uvs[4] = { {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f} };
        const f32 su[4] = { -1.0f,  1.0f,  1.0f, -1.0f };
        const f32 sv[4] = { -1.0f, -1.0f,  1.0f,  1.0f };

        const u32 base = static_cast<u32>(vertices.size());
        for (u32 k = 0; k < 4; ++k) {
            Vertex v;
            v.position = f.n * h + f.t * (su[k] * h) + b * (sv[k] * h);
            v.normal   = f.n;
            v.uv       = uvs[k];
            v.tangent  = f.t;
            vertices.push_back(v);
        }
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
        indices.push_back(base + 0);
    }

    return buildModel(vertices, indices, materialName);
}

Model Sphere(const std::string& materialName, f32 radius, u32 sectors, u32 stacks) {
    if (sectors < 3) sectors = 3;
    if (stacks < 2) stacks = 2;

    std::vector<Vertex> vertices;
    vertices.reserve((sectors + 1) * (stacks + 1));

    for (u32 i = 0; i <= stacks; ++i) {
        const f32 phi = static_cast<f32>(i) / static_cast<f32>(stacks) * PI; // 0..PI (polo +Y a -Y)
        const f32 y   = std::cos(phi);
        const f32 r   = std::sin(phi);
        for (u32 j = 0; j <= sectors; ++j) {
            const f32 theta = static_cast<f32>(j) / static_cast<f32>(sectors) * TAU;
            const f32 ct = std::cos(theta);
            const f32 st = std::sin(theta);

            Vertex v;
            v.normal   = { r * ct, y, r * st };
            v.position = v.normal * radius;
            v.uv       = { static_cast<f32>(j) / static_cast<f32>(sectors),
                           static_cast<f32>(i) / static_cast<f32>(stacks) };
            v.tangent  = { -st, 0.0f, ct }; // d/dtheta, tangente a la longitud
            vertices.push_back(v);
        }
    }

    std::vector<u32> indices;
    indices.reserve(sectors * stacks * 6);
    stitchGrid(indices, stacks + 1, sectors);

    return buildModel(vertices, indices, materialName);
}

Model Cylinder(const std::string& materialName, f32 radius, f32 height, u32 sectors) {
    if (sectors < 3) sectors = 3;

    const f32 halfH = height * 0.5f;

    std::vector<Vertex> vertices;
    std::vector<u32> indices;

    // --- Pared lateral: dos anillos (abajo/arriba) con normal radial. ---
    const u32 sideBase = static_cast<u32>(vertices.size());
    for (u32 ring = 0; ring < 2; ++ring) {
        const f32 y = (ring == 0) ? -halfH : halfH;
        for (u32 j = 0; j <= sectors; ++j) {
            const f32 theta = static_cast<f32>(j) / static_cast<f32>(sectors) * TAU;
            const f32 ct = std::cos(theta);
            const f32 st = std::sin(theta);

            Vertex v;
            v.position = { radius * ct, y, radius * st };
            v.normal   = { ct, 0.0f, st };
            v.uv       = { static_cast<f32>(j) / static_cast<f32>(sectors),
                           static_cast<f32>(ring) };
            v.tangent  = { -st, 0.0f, ct };
            vertices.push_back(v);
        }
    }
    for (u32 j = 0; j < sectors; ++j) {
        const u32 a = sideBase + j;                 // anillo inferior
        const u32 b = a + (sectors + 1);            // anillo superior
        indices.push_back(a);     indices.push_back(b);     indices.push_back(a + 1);
        indices.push_back(a + 1); indices.push_back(b);     indices.push_back(b + 1);
    }

    // --- Tapas: un abanico de triángulos por tapa. ---
    auto addCap = [&](f32 y, f32 ny) {
        const u32 center = static_cast<u32>(vertices.size());
        {
            Vertex c;
            c.position = { 0.0f, y, 0.0f };
            c.normal   = { 0.0f, ny, 0.0f };
            c.uv       = { 0.5f, 0.5f };
            c.tangent  = { 1.0f, 0.0f, 0.0f };
            vertices.push_back(c);
        }
        const u32 ringBase = static_cast<u32>(vertices.size());
        for (u32 j = 0; j <= sectors; ++j) {
            const f32 theta = static_cast<f32>(j) / static_cast<f32>(sectors) * TAU;
            const f32 ct = std::cos(theta);
            const f32 st = std::sin(theta);

            Vertex v;
            v.position = { radius * ct, y, radius * st };
            v.normal   = { 0.0f, ny, 0.0f };
            v.uv       = { ct * 0.5f + 0.5f, st * 0.5f + 0.5f };
            v.tangent  = { 1.0f, 0.0f, 0.0f };
            vertices.push_back(v);
        }
        for (u32 j = 0; j < sectors; ++j) {
            indices.push_back(center);
            // Orden según la cara para mantener el frente hacia afuera.
            if (ny > 0.0f) {
                indices.push_back(ringBase + j);
                indices.push_back(ringBase + j + 1);
            } else {
                indices.push_back(ringBase + j + 1);
                indices.push_back(ringBase + j);
            }
        }
    };
    addCap( halfH,  1.0f);
    addCap(-halfH, -1.0f);

    return buildModel(vertices, indices, materialName);
}

Model Capsule(const std::string& materialName, f32 radius, f32 height, u32 sectors, u32 rings) {
    if (sectors < 3) sectors = 3;
    if (rings < 1)   rings = 1;

    const f32 halfH = height * 0.5f;

    std::vector<Vertex> vertices;

    // Emite una fila (anillo) de vértices en función de phi (para la normal) y
    // el desplazamiento vertical del centro del casquete.
    auto addRow = [&](f32 phi, f32 centerY) {
        const f32 cp = std::cos(phi);
        const f32 sp = std::sin(phi);
        for (u32 j = 0; j <= sectors; ++j) {
            const f32 theta = static_cast<f32>(j) / static_cast<f32>(sectors) * TAU;
            const f32 ct = std::cos(theta);
            const f32 st = std::sin(theta);

            Vertex v;
            v.normal   = { sp * ct, cp, sp * st };
            v.position = { radius * sp * ct, centerY + radius * cp, radius * sp * st };
            v.uv       = { static_cast<f32>(j) / static_cast<f32>(sectors), 0.0f };
            v.tangent  = { -st, 0.0f, ct };
            vertices.push_back(v);
        }
    };

    // Casquete superior: phi 0..PI/2, centro en +halfH.
    for (u32 i = 0; i <= rings; ++i) {
        const f32 phi = static_cast<f32>(i) / static_cast<f32>(rings) * (PI * 0.5f);
        addRow(phi, halfH);
    }
    // Casquete inferior: phi PI/2..PI, centro en -halfH. En el seam (phi=PI/2)
    // ambas filas quedan en el radio máximo, formando la pared del cilindro.
    for (u32 i = 0; i <= rings; ++i) {
        const f32 phi = (PI * 0.5f) + static_cast<f32>(i) / static_cast<f32>(rings) * (PI * 0.5f);
        addRow(phi, -halfH);
    }

    const u32 rows = 2 * (rings + 1);
    // Coordenada V acumulada a lo largo del eje para UVs razonables.
    for (u32 i = 0; i < rows; ++i) {
        const f32 vCoord = static_cast<f32>(i) / static_cast<f32>(rows - 1);
        for (u32 j = 0; j <= sectors; ++j) {
            vertices[i * (sectors + 1) + j].uv.y = vCoord;
        }
    }

    std::vector<u32> indices;
    indices.reserve(rows * sectors * 6);
    stitchGrid(indices, rows, sectors);

    return buildModel(vertices, indices, materialName);
}

}
}
}
