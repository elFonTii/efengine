#pragma once
#include <efengine/renderer/IScenePass.h>
#include <efengine/renderer/ClusterMath.h>
#include <efengine/renderer/ClusterSettings.h>
#include <efengine/renderer/StorageBuffer.h>

#include <glm/glm.hpp>
#include <memory>
#include <optional>

namespace efengine {
namespace renderer {

    class Shader;

    // Arma la grilla de clusters del frame y corre el compute que asigna luces a
    // cada cluster. Va despues de FrameUploadPass (el compute lee uView del
    // bloque Frame) y antes del forward.
    class ClusterLightPass : public IScenePass {
        public:
            // null si falta el shader: el frame sigue y pbr.frag recorre todas
            // las visibles (misma imagen, mas lenta).
            static std::unique_ptr<ClusterLightPass> Create(const Shader* cull);

            void Execute(FrameContext& ctx) override;

            const char* Name() const override { return "Clusters"; }

            ClusterSettings&       settings()       { return m_settings; }
            const ClusterSettings& settings() const { return m_settings; }
            const ClusterGrid&     grid()     const { return m_grid; }
            u32                    aabbRebuilds() const { return m_aabbRebuilds; }

        private:
            explicit ClusterLightPass(const Shader* cull) : m_cull(cull) {}

            const Shader*   m_cull = null;
            u32             m_aabbRebuilds = 0u;
            ClusterSettings m_settings;
            ClusterGrid     m_grid;
            bool            m_hayGrilla = false;
            glm::mat4       m_proyeccion { 0.0f };

            // optional: StorageBuffer no tiene constructor vacio y el tamano sale de la grilla.
            std::optional<StorageBuffer> m_aabbs;
            std::optional<StorageBuffer> m_listas;
    };

}
}
