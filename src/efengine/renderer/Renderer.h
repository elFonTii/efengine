#pragma once

#include <efecom/RHI.h>
#include <efengine/core/Types.h>
#include <efengine/renderer/VertexArray.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/Material.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/PointLight.h>
#include <efengine/renderer/DirectionalLight.h>
#include <efengine/renderer/ShadowContext.h>
#include <efengine/renderer/IblContext.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/DdgiContext.h>
#include <efengine/renderer/UniformBuffer.h>

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace efengine {
namespace renderer {

    class Cubemap;

    class Renderer {
        public:
            static constexpr u32 kMaxLights = 4; // DEBE COINCIDIR CON MAX_LIGHTS DEL SHADER PRINCIPAL

            // Crea los 4 UBOs de escena y los engancha a sus bindings. Necesita
            // contexto GL: en Application se declara despues de Context.
            Renderer();

            void Clear(f32 r, f32 g, f32 b, f32 a) const;
            void SetViewport(u32 width, u32 height) const; // por el momento para evitar que Application llame gl crudo
            void Draw(const VertexArray& va, const Shader& shader) const;

            void BeginScene(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& viewPos, const std::vector<PointLight>& lights, const DirectionalLight& sun, const ShadowContext& shadow, const IblContext& ibl, const DdgiContext& ddgi);

            // overrideShader != null dibuja TODO con ese programa en vez del del
            // material, pero sigue subiendo el MaterialBlock y bindeando las
            // texturas. Lo usa la captura de probes de DDGI, que necesita el
            // albedo de cada material pero un solo shader difuso.
            //
            // overrideState fuerza el estado de rasterizacion e ignora
            // mat.doubleSided. Va aparte de overrideShader a proposito: dibujar
            // con otro shader no implica dibujar con otro estado, y acoplarlos
            // dejaria sin estado a cualquier otro consumidor de overrideShader.
            void Submit(const Model& model, const MaterialMap& materials, const glm::mat4& modelMatrix,
                        const Shader* overrideShader = null,
                        const efecom::PipelineState* overrideState = null);

            // Sube la matriz de modelo al bloque Object (binding 2). Publico
            // porque ShadowPass tambien dibuja por objeto y necesita el mismo bloque.
            void SetObjectMatrix(const glm::mat4& model) const;

            // Sube un bloque Frame arbitrario (binding 0). Publico porque la
            // captura de probes de DDGI lo re-sube seis veces por probe, con la
            // view/proj de cada cara. Por eso ese pase corre ANTES de BeginScene:
            // si corriera despues, pisaria la camara.
            void SetFrameBlock(const FrameBlock& block) const;

            // Sube un bloque Ddgi arbitrario (binding 5). Publico porque los
            // blends de DDGI corren antes de BeginScene y necesitan el bloque con
            // SU updateRange y su hysteresis forzada del primer barrido, que no
            // son los que el frame le va a dar despues a pbr.frag.
            void SetDdgiBlock(const DdgiBlock& block) const;

        private:
            // Un UBO por frecuencia de actualizacion. El de material vive aca y no
            // en Material a proposito: Material se construye headless en los tests
            // y un handle de GPU adentro lo rompe.
            UniformBuffer m_frameUbo;
            UniformBuffer m_lightsUbo;
            UniformBuffer m_objectUbo;
            UniformBuffer m_materialUbo;
            UniformBuffer m_ddgiUbo;
    };

}
}
