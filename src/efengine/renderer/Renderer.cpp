#include "Renderer.h"

#include <efecom/RHI.h>

#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/Cubemap.h>
#include <efengine/renderer/PipelineStates.h>


namespace efengine {
namespace renderer {

    void Renderer::Clear(f32 r, f32 g, f32 b, f32 a) const {
        efecom::SetClearColor(r, g, b, a);
        efecom::Clear(efecom::ClearMask::ColorDepth);
    }

    void Renderer::SetViewport(u32 width, u32 height) const { efecom::SetViewport(0, 0, width, height); }

    void Renderer::Draw(const VertexArray& va, const Shader& shader) const {
        EF_ASSERT(va.vertexCount() > 0, "Renderer::Draw: VertexArray sin vertices");

        shader.Bind();
        va.Bind();
        
        if(va.hasIndexBuffer()) {
            efecom::DrawIndexed(va.indexCount());
        } else {
            efecom::DrawArrays(va.vertexCount());
        }

        efecom::BindVertexArray(0);
        efecom::BindProgram(0);
    }

    void Renderer::BeginScene(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& viewPos, const std::vector<PointLight>& lights, const DirectionalLight& sun, const ShadowContext& shadow, const IblContext& ibl) {
        // inicializacion simplemente
        m_view = view;
        m_projection = projection;
        m_viewPos = viewPos;
        m_lights.assign(lights.begin(), lights.end());
        m_sun = sun;
        m_shadow = shadow;
        m_ibl = ibl;

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
        shader.SetInt("uLightCount", static_cast<i32>(m_lights.size()));
        
        // recorrer luces, construir nombre y agregar
        for (u32 i = 0; i < m_lights.size(); ++i) {
            const std::string lightName = "uLightPositions[" + std::to_string(i) + "]";
            const std::string lightColor= "uLightColors[" + std::to_string(i) + "]";

            shader.SetVec3(lightName.c_str(), m_lights[i].position);
            shader.SetVec3(lightColor.c_str(), m_lights[i].color);

        }

        // Luz direccional (sol)
        shader.SetVec3("uLightDir", m_sun.direction);
        shader.SetVec3("uDirLightColor", m_sun.color);

        // Sombra direccional (shadow map en unit 8: las 0-7 son del material)
        shader.SetMat4("uLightSpaceMatrix", m_shadow.lightSpaceMatrix);
        shader.SetInt("uShadowEnabled", m_shadow.enabled ? 1 : 0);
        shader.SetFloat("uShadowBiasMin", m_shadow.biasMin);
        shader.SetFloat("uShadowBiasMax", m_shadow.biasMax);
        if (m_shadow.map != null) {
            m_shadow.map->Bind(8);
            shader.SetInt("uShadowMap", 8);
        }

        // IBL: irradiancia 9, prefiltrado 10, BRDF LUT 11. Los tres o ninguno.
        const bool hasIbl = (m_ibl.irradiance != null
                          && m_ibl.prefiltered != null
                          && m_ibl.brdfLut != null);
        shader.SetInt("uHasIbl", hasIbl ? 1 : 0);
        shader.SetFloat("uIblIntensity", m_ibl.intensity);
        if (hasIbl) {
            m_ibl.irradiance->Bind(9);
            shader.SetInt("uIrradianceMap", 9);
            m_ibl.prefiltered->Bind(10);
            shader.SetInt("uPrefilterMap", 10);
            m_ibl.brdfLut->Bind(11);
            shader.SetInt("uBrdfLUT", 11);
            shader.SetFloat("uPrefilterMaxLod", m_ibl.maxLod);
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

            // El estado sale del material.
            efecom::ApplyPipelineState(mat.doubleSided ? OpaqueDoubleSidedState()
                                                       : OpaqueState());

            applyFrameUniforms(shader);
            shader.Bind();
            shader.SetMat4("uModel", modelMatrix);
            mat.BindTextures();
            // TODO(Task 9): esto sale del MaterialBlock por UBO.
            shader.SetVec3 ("uAlbedoTint",        mat.albedoTint);
            shader.SetFloat("uMetallic",          mat.metallic);
            shader.SetFloat("uRoughness",         mat.roughness);
            shader.SetFloat("uAOStrength",        mat.aoStrength);
            shader.SetFloat("uHeightScale",       mat.heightScale);
            shader.SetFloat("uAlphaCutoff",       mat.alphaCutoff);
            shader.SetVec3 ("uEmissiveTint",      mat.emissiveTint);
            shader.SetFloat("uEmissiveIntensity", mat.emissiveIntensity);
            shader.SetFloat("uNormalStrength",    mat.normalStrength);
            {
                const MaterialBlock blk = mat.ToBlock();
                shader.SetInt("uHasAlbedoMap",    (blk.mapMask.x & (1u << 0)) ? 1 : 0);
                shader.SetInt("uHasNormalMap",    (blk.mapMask.x & (1u << 1)) ? 1 : 0);
                shader.SetInt("uHasAOMap",        (blk.mapMask.x & (1u << 2)) ? 1 : 0);
                shader.SetInt("uHasRoughnessMap", (blk.mapMask.x & (1u << 3)) ? 1 : 0);
                shader.SetInt("uHasMetallicMap",  (blk.mapMask.x & (1u << 4)) ? 1 : 0);
                shader.SetInt("uHasHeightMap",    (blk.mapMask.x & (1u << 5)) ? 1 : 0);
                shader.SetInt("uHasOpacityMap",   (blk.mapMask.x & (1u << 6)) ? 1 : 0);
                shader.SetInt("uHasEmissiveMap",  (blk.mapMask.x & (1u << 7)) ? 1 : 0);
                shader.SetInt("uAlbedoMap",    0); shader.SetInt("uNormalMap",    1);
                shader.SetInt("uAOMap",        2); shader.SetInt("uRoughnessMap", 3);
                shader.SetInt("uMetallicMap",  4); shader.SetInt("uHeightMap",    5);
                shader.SetInt("uOpacityMap",   6); shader.SetInt("uEmissiveMap",  7);
            }

            Draw(mesh.vertexArray(), shader);
            }
    };
}
}
