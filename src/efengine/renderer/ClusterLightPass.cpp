#include "efengine/renderer/ClusterLightPass.h"

#include <efecom/RHI.h>

#include <efengine/renderer/FrameContext.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/scene/Camera.h>

#include <vector>

namespace efengine {
namespace renderer {

    namespace {
        constexpr u32 kHilosPorGrupo = 128u;   // local_size_x de cluster_cull.comp
    }

    std::unique_ptr<ClusterLightPass> ClusterLightPass::Create(const Shader* cull) {
        if (cull == null) return nullptr;
        return std::unique_ptr<ClusterLightPass>(new ClusterLightPass(cull));
    }

    void ClusterLightPass::Execute(FrameContext& ctx) {
        const glm::mat4   proyeccion = ctx.view.projectionNoJitter;
        const ClusterGrid grid = MakeClusterGrid(m_settings, ctx.width, ctx.height,
                                                 ctx.camera.NearPlane(), ctx.camera.FarPlane());

        const bool cambioTamano = !m_hayGrilla || grid.Count() != m_grid.Count()
                               || grid.maxLightsPerCluster != m_grid.maxLightsPerCluster;
        const bool cambioCajas  = cambioTamano || !SameGrid(grid, m_grid) || proyeccion != m_proyeccion;

        if (cambioTamano) {
            const usize clusters = static_cast<usize>(grid.Count());
            m_aabbs.emplace(clusters * sizeof(ClusterAabb));
            m_listas.emplace(clusters * (grid.maxLightsPerCluster + 1u) * sizeof(u32));
        }
        if (cambioCajas) {
            ++m_aabbRebuilds;
            const std::vector<ClusterAabb> cajas =
                BuildClusterAabbs(grid, glm::inverse(proyeccion), ctx.width, ctx.height);
            m_aabbs->Update(cajas.data(), cajas.size() * sizeof(ClusterAabb));
            m_grid       = grid;
            m_proyeccion = proyeccion;
            m_hayGrilla  = true;
        }

        m_aabbs->BindTo(kClusterAabbsBinding);
        m_listas->BindTo(kClusterLightsBinding);
        ctx.renderer.SetClusterBlock(MakeClusterBlock(m_grid, m_settings.debugView));

        m_cull->Bind();
        efecom::DispatchCompute((m_grid.Count() + kHilosPorGrupo - 1u) / kHilosPorGrupo, 1u, 1u);
        // pbr.frag lee las listas por SSBO: sin esto puede leerlas a medio escribir.
        efecom::IssueMemoryBarrier(efecom::Barrier::ShaderStorage);
    }

}
}
