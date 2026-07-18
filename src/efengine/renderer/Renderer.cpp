#include "Renderer.h"

#include <glad/gl.h>

#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>


namespace efengine {
namespace renderer {

    void Renderer::Clear(f32 r, f32 g, f32 b, f32 a) const {
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

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

    void Renderer::BeginScene(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& viewPos, const std::vector<PointLight>& lights, f32 ambientFactor) {
        // inicializacion simplemente
        m_view = view;
        m_projection = projection;
        m_ambient = ambientFactor;
        m_viewPos = viewPos;
        m_lights.assign(lights.begin(), lights.end());

        // si la cantidad de luces es mayor a las soportadas por el shader recortar
        if(m_lights.size() > kMaxLights) {
            m_lights.resize(kMaxLights);
            EF_LOG_WARNING("Se intentan agregar más luces de las que el shader soporta.");
        }

        // Se limpia antes de registrar en cada frame
        m_frameShaders.clear();
    }

    // por frame
    void Renderer::applyFrameUniforms(const Shader& shader) {
        if (!m_frameShaders.insert(&shader).second) return; // para evitar duplicados, si no es nuevo sale.

        shader.Bind();
        shader.SetMat4("uView", m_view);
        shader.SetMat4("uProjection", m_projection);
        shader.SetVec3("uViewPos", m_viewPos);
        shader.SetFloat("uAmbientFactor", m_ambient);
        shader.SetInt("uLightCount", static_cast<i32>(m_lights.size()));
        
        // recorrer luces, construir nombre y agregar
        for (u32 i = 0; i < m_lights.size(); ++i) {
            const std::string lightName = "uLightPositions[" + std::to_string(i) + "]";
            const std::string lightColor= "uLightColors[" + std::to_string(i) + "]";

            shader.SetVec3(lightName.c_str(), m_lights[i].position);
            shader.SetVec3(lightColor.c_str(), m_lights[i].color);
            
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
