#include "Renderer.h"

#include <efecom/RHI.h>

#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/Cubemap.h>
#include <efengine/renderer/PipelineStates.h>
#include <efengine/renderer/DdgiSettings.h>
#include <efengine/renderer/AoMath.h>

namespace efengine {
namespace renderer {

    Renderer::Renderer()
        : m_frameUbo(sizeof(FrameBlock))
        , m_lightsUbo(sizeof(LightsBlock))
        , m_objectUbo(sizeof(ObjectBlock))
        , m_materialUbo(sizeof(MaterialBlock))
        , m_ddgiUbo(sizeof(DdgiBlock))
        , m_aoUbo(sizeof(AoBlock)) {
        // glBindBufferBase es estado GLOBAL, no por programa: alcanza engancharlos
        // una vez aca. Por eso desaparecio el set m_frameShaders, que existia solo
        // para no re-setear los mismos uniforms en cada programa del frame.
        m_frameUbo.BindTo(kFrameBinding);
        m_lightsUbo.BindTo(kLightsBinding);
        m_objectUbo.BindTo(kObjectBinding);
        m_materialUbo.BindTo(kMaterialBinding);
        m_ddgiUbo.BindTo(kDdgiBinding);
        m_aoUbo.BindTo(kAoBinding);
    }

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

        // Ya no se desbindea programa ni VAO: cada pase bindea lo suyo antes de
        // dibujar. Desbindear obligaba a ShadowPass a re-bindear su shader por
        // objeto para no perder los uniforms, que con UBOs ni siquiera aplica.
    }

    void Renderer::BeginScene(const glm::mat4& view, const glm::mat4& projection,
                              const glm::vec3& viewPos, const std::vector<PointLight>& lights,
                              const DirectionalLight& sun, const SceneLighting& lighting) {
        const ShadowContext& shadow = lighting.shadow;
        const IblContext&    ibl    = lighting.ibl;
        const DdgiContext&   ddgi   = lighting.ddgi;

        if (lights.size() > kMaxLights) {
            EF_LOG_WARNING("Se intentan agregar más luces de las que el shader soporta.");
        }

        // Los mapas de frame van a sus unidades fijas. Los samplers ya saben su
        // unidad por layout(binding=N): aca solo se bindea la textura.
        if (shadow.map != null) shadow.map->Bind(8);
        if (ibl.irradiance != null && ibl.prefiltered != null && ibl.brdfLut != null) {
            ibl.irradiance->Bind(9);
            ibl.prefiltered->Bind(10);
            ibl.brdfLut->Bind(11);
        }

        const FrameBlock  frameBlock  = MakeFrameBlock(view, projection, viewPos, shadow, ibl);
        const LightsBlock lightsBlock = MakeLightsBlock(lights, sun);

        m_frameUbo.Update(&frameBlock, sizeof(frameBlock));
        m_lightsUbo.Update(&lightsBlock, sizeof(lightsBlock));

        // Los dos atlas de DDGI a sus unidades fijas. Si falta cualquiera, no se
        // bindea nada y el bloque apaga DDGI: samplear una unidad vacia da
        // resultados indefinidos, no negro.
        const bool atlasValid = (ddgi.irradianceAtlas != null && ddgi.distanceAtlas != null
                                 && ddgi.settings != null);
        if (atlasValid) {
            ddgi.irradianceAtlas->Bind(kIrradianceAtlasUnit);
            ddgi.distanceAtlas->Bind(kDistanceAtlasUnit);
        }

        const DdgiSettings defaults {};
        const DdgiSettings& ds = atlasValid ? *ddgi.settings : defaults;
        const DdgiBlock ddgiBlock = MakeDdgiBlock(ds.grid, ds, ddgi.range, atlasValid);
        m_ddgiUbo.Update(&ddgiBlock, sizeof(ddgiBlock));

        // El AO a su unidad fija. Si no hay textura, MakeAoBlock apaga el bloque
        // y pbr.frag ni la samplea.
        if (lighting.ao.texture != null) lighting.ao.texture->Bind(kAoTextureUnit);
        const AoBlock aoBlock = MakeAoBlock(lighting.ao);
        m_aoUbo.Update(&aoBlock, sizeof(aoBlock));
    }

    void Renderer::SetFrameBlock(const FrameBlock& block) const {
        m_frameUbo.Update(&block, sizeof(block));
    }

    void Renderer::SetDdgiBlock(const DdgiBlock& block) const {
        m_ddgiUbo.Update(&block, sizeof(block));
    }

    void Renderer::SetObjectMatrix(const glm::mat4& model) const {
        const ObjectBlock block { model };
        m_objectUbo.Update(&block, sizeof(block));
    }

    void Renderer::Submit(const Model& model, const MaterialMap& materials, const glm::mat4& modelMatrix,
                          const Shader* overrideShader, const efecom::PipelineState* overrideState) {
        // La matriz de modelo es del render item entero: se sube UNA vez, no una
        // por submesh como hacia el uModel viejo.
        SetObjectMatrix(modelMatrix);

        for (const Mesh& mesh : model.meshes()) {
            auto it = materials.find(mesh.materialName());
            if (it == materials.end() || it->second == null) {
                EF_LOG_WARNING("Renderer::Submit: sin material para malla '%s'", mesh.materialName().c_str());
                continue;
            }
            const Material& mat = *it->second;

            efecom::ApplyPipelineState(overrideState != null
                                     ? *overrideState
                                     : (mat.doubleSided ? OpaqueDoubleSidedState()
                                                        : OpaqueState()));

            const MaterialBlock block = mat.ToBlock();
            m_materialUbo.Update(&block, sizeof(block));
            mat.BindTextures();

            Draw(mesh.vertexArray(), (overrideShader != null) ? *overrideShader : mat.shader());
        }
    }
}
}
