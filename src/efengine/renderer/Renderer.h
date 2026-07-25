#pragma once

#include <efengine/core/Types.h>
#include <efengine/renderer/VertexArray.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/Material.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/PointLight.h>
#include <efengine/renderer/DirectionalLight.h>
#include <efengine/renderer/ShadowContext.h>
#include <efengine/renderer/FrameData.h>
#include <efengine/renderer/UniformBuffer.h>

#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace efengine {
namespace renderer {

    class Cubemap;

    class Renderer {
        public:
            static constexpr u32 kMaxLights = kFrameMaxLights; // el contrato vive en FrameData.h
            static constexpr u32 kFrameDataBinding = 0; // binding del UBO FrameData en los shaders

            void Clear(f32 r, f32 g, f32 b, f32 a) const;
            void SetViewport(u32 width, u32 height) const; // por el momento para evitar que Application llame gl crudo
            void Draw(const Model& va, const Shader& shader) const;
            void Draw(const Model& model, const MaterialMap& materials) const;
            void Draw(const VertexArray& va, const Shader& shader) const;
            void BeginScene(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& viewPos, const std::vector<PointLight>& lights, f32 ambientFactor, const DirectionalLight& sun, const ShadowContext& shadow, const Cubemap* irradiance);
            void Submit(const Model& model, const MaterialMap& materials, const glm::mat4& modelMatrix);

        private:
            void applyFrameUniforms(const Shader& shader);

            // UBO con los datos per-frame (binding 0). Lazy: se crea en el
            // primer BeginScene, cuando ya hay contexto GL seguro.
            std::optional<UniformBuffer> m_frameUbo;
            ShadowContext m_shadow {};
            const Cubemap* m_irradiance = nullptr;
            std::unordered_set<const Shader*> m_frameShaders;
    };

}
}
