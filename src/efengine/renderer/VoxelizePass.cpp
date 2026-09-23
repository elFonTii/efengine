#include "efengine/renderer/VoxelizePass.h"

#include <efengine/core/Assert.h>
#include <efengine/renderer/Bounds.h>
#include <efengine/renderer/Material.h>
#include <efengine/renderer/Mesh.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/PipelineStates.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/VoxelMath.h>
#include <efengine/scene/SceneGraph.h>
#include <efecom/RHI.h>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <chrono>
#include <functional>

namespace efengine {
namespace renderer {

    namespace {
        // El mismo patron de DdgiPass: mide hasta el fin del scope, asi un
        // retorno temprano no deja m_lastMs con el valor de la corrida anterior.
        struct ScopedMs {
            f32* out;
            std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
            ~ScopedMs() {
                *out = std::chrono::duration<f32, std::milli>(
                           std::chrono::steady_clock::now() - t0).count();
            }
        };
    }

    VoxelizePass::VoxelizePass(Renderer& renderer, Shader* voxelize)
                : m_renderer(renderer), m_voxelize(voxelize) {
                    EF_ASSERT(voxelize != null, "VoxelizePass: Se intenta inyectar shader nulo");
                }

    VoxelizePass::~VoxelizePass() {
        if (m_fbo != 0u) efecom::DestroyFramebuffer(m_fbo);
    }

    std::unique_ptr<VoxelizePass> VoxelizePass::Create(Renderer& renderer, Shader* voxelize) {
        if (voxelize == null) return nullptr;
        return std::unique_ptr<VoxelizePass>(new VoxelizePass(renderer, voxelize));
    }

    void VoxelizePass::Execute(const scene::SceneGraph& scene, VoxelGrid& grid) {
        const ScopedMs medicion { &m_lastMs };

        m_lastDraws = 0u;
        if (!grid.valid()) return;

        grid.Clear();
        grid.BindForWrite();

        const VoxelGridDesc& desc = grid.desc();
        const AABB  b = GridBounds(desc);
        const glm::vec3 c = b.Center();
        const f32   h = 0.5f * GridExtent(desc);

        // El rasterizador necesita un area de barrido del tamano del grid: cada
        // pixel de esa grilla es una columna de voxeles. Sale del FBO propio sin
        // attachments y NO del target de presentacion, que ataria la cobertura
        // del grid al tamano de la ventana.
        if (m_fbo == 0u) m_fbo = efecom::CreateFramebuffer();
        if (m_fboRes != desc.resolution) {
            efecom::FramebufferDefaultSize(m_fbo, desc.resolution, desc.resolution);
            m_fboRes = desc.resolution;
            EF_ASSERT(efecom::FramebufferComplete(m_fbo),
                      "VoxelizePass: el FBO sin attachments quedo incompleto");
        }
        efecom::BindRenderTarget(m_fbo, desc.resolution, desc.resolution);

        const glm::mat4 orto = glm::ortho(-h, h, -h, h, 0.0f, 2.0f * h);
        const glm::mat4 vistas[3] = {
            orto * glm::lookAt(c + glm::vec3(h, 0, 0), c, glm::vec3(0, 1, 0)),  // mirando -X
            // El up del eje Y es (0,0,1) y no (0,1,0) a proposito: lookAt con la
            // direccion paralela al up produce una matriz degenerada de NaN.
            orto * glm::lookAt(c + glm::vec3(0, h, 0), c, glm::vec3(0, 0, 1)),  // mirando -Y
            orto * glm::lookAt(c + glm::vec3(0, 0, h), c, glm::vec3(0, 1, 0)),  // mirando -Z
        };

        // Sin culling: la voxelizacion quiere la escena ENTERA. Un objeto fuera
        // del frustum de una pasada sigue estando dentro del grid, y descartarlo
        // dejaria un agujero que el trazado ve como aire.
        const std::vector<scene::RenderItem>& items = scene.Renderables();
        const std::vector<MeshSpan>&          spans = scene.MeshSpans();

        m_draws.clear();
        for (const MeshSpan& span : spans) {
            EF_ASSERT(span.item < static_cast<u32>(items.size()),
                      "VoxelizePass: span.item fuera de Renderables");

            const scene::RenderItem& item = items[span.item];
            if (item.model == null || item.materials == null) continue;

            EF_ASSERT(span.mesh < static_cast<u32>(item.model->meshes().size()),
                      "VoxelizePass: span.mesh fuera de las submallas del modelo");

            const Mesh& malla = item.model->meshes()[span.mesh];
            auto it = item.materials->find(malla.materialName());
            if (it == item.materials->end() || it->second == null) continue;

            BatchDraw d;
            d.va       = &malla.vertexArray();
            d.material = it->second;
            d.world    = &item.world;
            m_draws.push_back(d);
        }

        // Por material y despues por objeto, que es lo que deja a SubmitBatch
        // saltear el MaterialBlock y la matriz de modelo repetidos. Por PUNTERO
        // y con std::less: comparar punteros a objetos distintos con < es
        // unspecified, y sin orden total stable_sort tiene UB.
        std::stable_sort(m_draws.begin(), m_draws.end(),
            [](const BatchDraw& a, const BatchDraw& b) {
                if (a.material != b.material) {
                    return std::less<const Material*>{}(a.material, b.material);
                }
                return std::less<const glm::mat4*>{}(a.world, b.world);
            });

        const efecom::PipelineState estado = VoxelizeState();
        DrawOptions opciones;
        opciones.shader = m_voxelize;
        opciones.state  = &estado;

        m_passUbo.BindTo(kPassBinding);
        for (const glm::mat4& viewProj : vistas) {
            const VoxelizePassBlock bloque {
                viewProj,
                glm::vec4(desc.origin, 0.0f),
                glm::vec4(desc.voxelSize, static_cast<f32>(desc.resolution), 0.0f, 0.0f)
            };
            m_passUbo.Update(&bloque, sizeof(bloque));

            m_lastDraws += m_renderer.SubmitBatch(m_draws, opciones).draws;
        }

        // Sin esto el trazado puede leer el grid antes de que los imageStore
        // sean visibles, y ve el contenido del Clear.
        efecom::IssueMemoryBarrier(efecom::Barrier::ShaderImageAccess |
                                   efecom::Barrier::TextureFetch);
    }

}
}
