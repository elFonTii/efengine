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
#include <efengine/renderer/SceneLighting.h>
#include <efengine/renderer/IndirectContext.h>
#include <efengine/renderer/UniformBuffer.h>

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace efengine {
namespace renderer {

    class Cubemap;

    // Como testea la profundidad un draw.
    //
    //   Write : GL_LESS escribiendo depth. Es lo normal.
    //   Equal : GL_EQUAL sin escribir. Solo es valido si un depth prepass
    //           ya lleno el buffer con ESTA camara y ESTA geometria; si
    //           no, no se dibuja absolutamente nada.
    //
    // Es un campo de DrawOptions y no un `state` forzado porque tiene que
    // respetar el doubleSided de cada material: el prepass culleo segun el
    // material, y un forward que culleara distinto testearia contra la
    // profundidad de un triangulo que el prepass nunca dibujo. Un `state`
    // forzado, por contrato, ignora el material.
    enum class DepthMode { Write, Equal };

    // Todo lo que un pase puede cambiarle a un Submit. Es un struct y no
    // una tira de parametros con default porque ya eran cuatro: en la
    // llamada, `null, null, modo` no dice nada de lo que hace.
    struct DrawOptions {
        // Dibuja TODO con este programa en vez del del material, pero
        // sigue subiendo el MaterialBlock y bindeando las texturas. Lo
        // usan la captura de probes de DDGI (necesita el albedo de cada
        // material pero un solo shader difuso) y el prepass del AO.
        const Shader* shader = null;

        // Fuerza el estado de rasterizacion e IGNORA mat.doubleSided. Va
        // aparte de `shader` a proposito: dibujar con otro programa no
        // implica dibujar con otro estado, y acoplarlos dejaria sin
        // estado a cualquier otro consumidor de `shader`.
        //
        // Tiene precedencia sobre `depth`: quien fuerza el estado entero
        // ya dijo como quiere el depth test.
        const efecom::PipelineState* state = null;

        DepthMode depth = DepthMode::Write;

        // Repeticiones del draw. El vertex shader distingue cada una por
        // gl_InstanceID; que significa cada instancia lo decide el, no
        // esto. Lo usa la captura de DDGI para dibujar las 6 caras de
        // cada probe con un solo draw.
        u32 instances = 1u;
    };



    class Renderer {
        public:
            static constexpr u32 kMaxLights = 4; // DEBE COINCIDIR CON MAX_LIGHTS DEL SHADER PRINCIPAL

            // Crea los 4 UBOs de escena y los engancha a sus bindings. Necesita
            // contexto GL: en Application se declara despues de Context.
            Renderer();

            void Clear(f32 r, f32 g, f32 b, f32 a) const;
            void SetViewport(u32 width, u32 height) const;
            void Draw(const VertexArray& va, const Shader& shader, u32 instances = 1u) const;

            // Los cuatro contextos de iluminacion viajan en un solo struct: con
            // el AO eran nueve parametros, y el proximo sistema habria sumado el
            // decimo. Ver SceneLighting.h.
            void BeginScene(const glm::mat4& view, const glm::mat4& projection,
                            const glm::vec3& viewPos, const std::vector<PointLight>& lights,
                            const DirectionalLight& sun, const SceneLighting& lighting);

            void Submit(const Model& model, const MaterialMap& materials, const glm::mat4& modelMatrix,
                        const DrawOptions& options = DrawOptions());

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

            // Re-sube el bloque de binding 6 (AO + upsample) y bindea el par de
            // texturas de la indirecta. Existe porque IndirectPass corre DESPUES
            // de BeginScene -- necesita el bloque Frame ya subido -- pero su
            // resultado lo consume pbr.frag, que lee el bloque que BeginScene ya
            // habia armado sin el.
            //
            // Sin esta segunda subida, upsample.x queda en cero y pbr.frag
            // samplea el volumen inline igual: el pase corre, escribe su target
            // y nadie lo lee. Falla en velocidad y no en imagen, que es
            // exactamente la clase de bug que no se nota.
            void SetIndirectContext(const AoContext& ao, const IndirectContext& indirect) const;

        private:
            // Un UBO por frecuencia de actualizacion. El de material vive aca y no
            // en Material a proposito: Material se construye headless en los tests
            // y un handle de GPU adentro lo rompe.
            UniformBuffer m_frameUbo;
            UniformBuffer m_lightsUbo;
            UniformBuffer m_objectUbo;
            UniformBuffer m_materialUbo;
            UniformBuffer m_ddgiUbo;
            UniformBuffer m_aoUbo;
    };

}
}
