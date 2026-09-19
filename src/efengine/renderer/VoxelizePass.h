#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/BatchDraw.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/UniformBuffer.h>
#include <efengine/renderer/VoxelGrid.h>

#include <memory>
#include <vector>

namespace efengine {
namespace scene { class SceneGraph; }
namespace renderer {

    class Renderer;
    class Shader;

    // Llena el grid de voxeles con la escena. NO es un IScenePass: no corre por
    // frame. Lo dispara la carga de la escena y el boton del panel.
    class VoxelizePass {
        public:
            static std::unique_ptr<VoxelizePass> Create(Renderer& renderer, Shader* voxelize);

            // Limpia el grid y lo rellena con tres pasadas ortograficas.
            void Execute(const scene::SceneGraph& scene, VoxelGrid& grid);

            f32 lastMs()    const { return m_lastMs; }
            u32 lastDraws() const { return m_lastDraws; }

        private:
            VoxelizePass(Renderer& renderer, Shader* voxelize);

            Renderer& m_renderer;
            Shader*   m_voxelize = null;

            UniformBuffer m_passUbo { sizeof(VoxelizePassBlock) };

            std::vector<BatchDraw> m_draws;
            f32 m_lastMs    = 0.0f;
            u32 m_lastDraws = 0u;
    };

}
}
