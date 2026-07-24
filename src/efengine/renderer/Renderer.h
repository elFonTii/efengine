#pragma once

#include <efengine/core/Types.h>
#include <efengine/renderer/VertexArray.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/Material.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/PointLight.h>
#include <efengine/renderer/DirectionalLight.h>
#include <efengine/renderer/ShadowContext.h>

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace efengine {
namespace renderer {

    class Cubemap;

    class Renderer {
        public:
            static constexpr u32 kMaxLights = 4; // DEBE COINCIDIR CON MAX_LIGHTS DEL SHADER PRINCIPAL

            void Clear(f32 r, f32 g, f32 b, f32 a) const;
            void SetViewport(u32 width, u32 height) const; // por el momento para evitar que Application llame gl crudo
            void Draw(const Model& va, const Shader& shader) const;
            void Draw(const Model& model, const MaterialMap& materials) const;
            void Draw(const VertexArray& va, const Shader& shader) const;
            void BeginScene(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& viewPos, const std::vector<PointLight>& lights, f32 ambientFactor, const DirectionalLight& sun, const ShadowContext& shadow, const Cubemap* irradiance);
            void Submit(const Model& model, const MaterialMap& materials, const glm::mat4& modelMatrix);

        private:
            void applyFrameUniforms(const Shader& shader);

            glm::mat4 m_view = { 1.0f };
            glm::mat4 m_projection = { 1.0f };
            glm::vec3 m_viewPos { 0.0f };
            f32 m_ambient = 0.0f;
            std::vector<PointLight> m_lights;
            DirectionalLight m_sun { glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f) };
            ShadowContext m_shadow {};
            const Cubemap* m_irradiance = nullptr;
            std::unordered_set<const Shader*> m_frameShaders;
    };

}
}
