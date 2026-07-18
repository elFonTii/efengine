#pragma once

#include <efengine/core/Types.h>
#include <efengine/renderer/VertexArray.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/Material.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/PointLight.h>


#include <string>
#include <unordered_map>

namespace efengine {
namespace renderer {

    class Renderer {
        public:
            static constexpr u32 kMaxLights = 4; // DEBE COINCIDIR CON MAX_LIGHTS DEL SHADER PRINCIPAL

            void Clear(f32 r, f32 g, f32 b, f32 a) const;
            void Draw(const Model& va, const Shader& shader) const;
            void Draw(const Model& model, const MaterialMap& materials) const;
            void Draw(const VertexArray& va, const Shader& shader) const;
            void BeginScene(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& viewPos, const std::vector<PointLight>& lights, f32 ambientFactor);
            void Submit(const Model& model, const MaterialMap& materials, const glm::mat4& modelMatrix);

        private:
            void applyFrameUniforms(const Shader& shader);
    };

}
}
