#include "Renderer.h"

#include <glad/gl.h>

#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/Cubemap.h>


namespace efengine {
namespace renderer {

    void Renderer::Clear(f32 r, f32 g, f32 b, f32 a) const {
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Renderer::SetViewport(u32 width, u32 height) const { glViewport(0,0, (GLsizei) width, (GLsizei) height); }

    void Renderer::Draw(const Model& model, const Shader& shader) const {
        for (const Mesh& mesh : model.meshes()) {
            Draw(mesh.vertexArray(), shader); // Realiza un dibujado por cada mesh dentro del fbx
        }
    }

    void Renderer::Draw(const Model& model, const MaterialMap& materials) const {
        for (const Mesh& mesh : model.meshes()) {
            auto it = materials.find(mesh.materialName());
            if (it == materials.end() || it->second == null) {
                EF_LOG_WARNING("Renderer::Draw: sin material para malla '%s'", mesh.materialName().c_str());
                continue;
            }
            const Material& mat = *it->second;
            mat.Bind();
            Draw(mesh.vertexArray(), mat.shader());
        }
    }

    void Renderer::Draw(const VertexArray& va, const Shader& shader) const {
        EF_ASSERT(va.vertexCount() > 0, "Renderer::Draw: VertexArray sin vertices");

        shader.Bind();
        va.Bind();
        
        if(va.hasIndexBuffer()) {
            glDrawElements(GL_TRIANGLES, (GLsizei)va.indexCount(), GL_UNSIGNED_INT, nullptr);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)va.vertexCount());
        }

        glBindVertexArray(0);
        glUseProgram(0);
    }

    void Renderer::BeginScene(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& viewPos, const std::vector<PointLight>& lights, f32 ambientFactor, const DirectionalLight& sun, const ShadowContext& shadow, const Cubemap* irradiance) {
        m_shadow = shadow;
        m_irradiance = irradiance;

        // Todos los datos per-frame viajan en un solo UBO std140 (binding 0):
        // una subida por frame en lugar de glUniform* por nombre y por shader.
        FrameData frame {};
        frame.view             = view;
        frame.proj             = projection;
        frame.viewProj         = projection * view;
        frame.lightSpaceMatrix = shadow.lightSpaceMatrix;
        frame.camPos           = glm::vec4(viewPos, 0.0f);
        frame.sunDirection     = glm::vec4(sun.direction, shadow.enabled ? 1.0f : 0.0f);
        frame.sunColor         = glm::vec4(sun.color, ambientFactor);
        frame.shadowParams     = glm::vec4(shadow.biasMin, shadow.biasMax, 0.0f, 0.0f);

        // si la cantidad de luces es mayor a las soportadas por el shader recortar
        u32 lightCount = static_cast<u32>(lights.size());
        if (lightCount > kMaxLights) {
            lightCount = kMaxLights;
            EF_LOG_WARNING("Se intentan agregar más luces de las que el shader soporta.");
        }
        for (u32 i = 0; i < lightCount; ++i) {
            frame.pointPositions[i] = glm::vec4(lights[i].position, 0.0f);
            frame.pointColors[i]    = glm::vec4(lights[i].color, 0.0f);
        }
        frame.counts = glm::ivec4(static_cast<i32>(lightCount), 0, 0, 0);

        if (!m_frameUbo) {
            m_frameUbo = UniformBuffer::Create(sizeof(FrameData));
            EF_ASSERT(m_frameUbo.has_value(), "Renderer::BeginScene: no se pudo crear el UBO FrameData");
        }
        m_frameUbo->Update(&frame, sizeof(FrameData));
        m_frameUbo->BindBase(kFrameDataBinding);

        // Se limpia antes de registrar en cada frame
        m_frameShaders.clear();
    }

    // por frame: lo que queda fuera del UBO son los samplers (estado de programa).
    void Renderer::applyFrameUniforms(const Shader& shader) {
        if (!m_frameShaders.insert(&shader).second) return; // para evitar duplicados, si no es nuevo sale.

        shader.Bind();

        // Sombra direccional (shadow map en unit 7)
        if (m_shadow.map != null) {
            m_shadow.map->Bind(7);
            shader.SetInt("uShadowMap", 7);
        }

        // IBL difuso (irradiance cubemap en unit 8)
        if (m_irradiance != null) {
            m_irradiance->Bind(8);
            shader.SetInt("uIrradianceMap", 8);
        }
    };

    // Recarga los shaders por objeto
    void Renderer::Submit(const Model& model, const MaterialMap& materials, const glm::mat4& modelMatrix) {
        for (const Mesh& mesh : model.meshes()) {
            auto it = materials.find(mesh.materialName());
            if (it == materials.end() || it->second == null) {
                EF_LOG_WARNING("Renderer::Submit: sin material para malla '%s'", mesh.materialName().c_str());
                continue;
            }
            const Material& mat = *it->second;
            const Shader& shader = mat.shader();

            applyFrameUniforms(shader);
            shader.Bind();
            shader.SetMat4("uModel", modelMatrix);
            mat.Bind();

            Draw(mesh.vertexArray(), shader);
            }
    };
}
}
