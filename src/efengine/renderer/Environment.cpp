#include "efengine/renderer/Environment.h"
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/UniformBuffer.h>
#include <efengine/renderer/Texture.h>
#include <efengine/core/Log.h>
#include <efengine/core/Assert.h>

#include <efecom/RHI.h>
#include <utility>



namespace efengine {
namespace renderer {

    std::optional<Environment> Environment::Create(const EnvironmentDesc& desc, const EnvironmentShaders& shaders) {
        // Con punteros el tipo ya no garantiza que estén: antes eran const Shader&.
        if (desc.hdrPath == null) {
            EF_LOG_ERROR("Environment::Create: hdrPath es null");
            return std::nullopt;
        }
        if (shaders.equirectToCube == null || shaders.irradianceConvolve == null
            || shaders.prefilterGGX == null || shaders.brdfLut == null) {
            EF_LOG_ERROR("Environment::Create: falta algún compute del precomputo IBL");
            return std::nullopt;
        }

        auto hdr = Texture::CreateHDR(desc.hdrPath);
        if (!hdr) {
            EF_LOG_ERROR("Environment::Create: no se pudo cargar el HDR '%s'", desc.hdrPath);
            return std::nullopt;
        }

        Cubemap env = Cubemap::Create(desc.faceSize, efecom::TextureFormat::RGBA16F, mipLevels(desc.faceSize));

        shaders.equirectToCube->Bind();
        hdr->Bind(0);
        env.BindImage(0, 0, efecom::ImageAccess::WriteOnly, efecom::TextureFormat::RGBA16F);

        const u32 groups = (desc.faceSize + 7u) / 8u;
        efecom::DispatchCompute(groups, groups, 6);

        efecom::IssueMemoryBarrier(efecom::Barrier::ShaderImageAccess | efecom::Barrier::TextureFetch);

        env.GenerateMips();

        // --- Segunda pasada: convolución difusa del entorno a irradiancia ---
        Cubemap irradiance = Cubemap::Create(desc.irradianceSize, efecom::TextureFormat::RGBA16F, 1);

        shaders.irradianceConvolve->Bind();
        env.Bind(0);
        irradiance.BindImage(0, 0, efecom::ImageAccess::WriteOnly, efecom::TextureFormat::RGBA16F);

        const u32 iGroups = (desc.irradianceSize + 7u) / 8u;
        efecom::DispatchCompute(iGroups, iGroups, 6);
        efecom::IssueMemoryBarrier(efecom::Barrier::ShaderImageAccess | efecom::Barrier::TextureFetch);

        // --- Tercera pasada: prefiltrado GGX, un dispatch por mip ---
        // Cada nivel guarda el entorno convolucionado con una rugosidad distinta;
        // en el frame la rugosidad del material elige el mip con textureLod.
        Cubemap prefiltered = Cubemap::Create(desc.prefilterSize,
                                              efecom::TextureFormat::RGBA16F,
                                              desc.prefilterMips);

        shaders.prefilterGGX->Bind();
        env.Bind(0);

        // PassParams del prefiltrado: x = rugosidad del mip, y = resolucion de
        // cara del env (para el mip bias). Se re-sube por mip.
        UniformBuffer prefilterUbo(sizeof(PostParamsBlock));
        prefilterUbo.BindTo(kPassBinding);

        for (u32 mip = 0u; mip < desc.prefilterMips; ++mip) {
            const u32 mipSize = desc.prefilterSize >> mip;
            const f32 roughness = (desc.prefilterMips > 1u)
                                ? static_cast<f32>(mip) / static_cast<f32>(desc.prefilterMips - 1u)
                                : 0.0f;

            const PostParamsBlock p {
                glm::vec4(roughness, static_cast<f32>(desc.faceSize), 0.0f, 0.0f) };
            prefilterUbo.Update(&p, sizeof(p));

            prefiltered.BindImage(0, mip, efecom::ImageAccess::WriteOnly,
                                  efecom::TextureFormat::RGBA16F);

            const u32 groups = (mipSize + 7u) / 8u;
            efecom::DispatchCompute(groups, groups, 6);
        }
        efecom::IssueMemoryBarrier(efecom::Barrier::ShaderImageAccess | efecom::Barrier::TextureFetch);

        // --- Cuarta pasada: BRDF integration LUT del split-sum ---
        // No depende del HDR: es una integral pura de (NdotV, roughness). Se recalcula
        // igual en cada arranque porque cuesta un dispatch y evita versionar un asset.
        Texture brdfLut = Texture::CreateStorage2D(desc.lutSize, desc.lutSize,
                                                  efecom::TextureFormat::RG16F);

        shaders.brdfLut->Bind();
        brdfLut.BindImage(0, 0, efecom::ImageAccess::WriteOnly, efecom::TextureFormat::RG16F);

        // Z = 1: es una imagen 2D plana, no las 6 caras de un cubemap.
        const u32 lGroups = (desc.lutSize + 7u) / 8u;
        efecom::DispatchCompute(lGroups, lGroups, 1);
        efecom::IssueMemoryBarrier(efecom::Barrier::ShaderImageAccess | efecom::Barrier::TextureFetch);

        EF_LOG_INFO("Environment: IBL precomputado desde '%s' (env %ux%u, irradiancia %ux%u, "
                    "prefiltrado %ux%u x%u mips, LUT %ux%u)",
                    desc.hdrPath, desc.faceSize, desc.faceSize,
                    desc.irradianceSize, desc.irradianceSize,
                    desc.prefilterSize, desc.prefilterSize, desc.prefilterMips,
                    desc.lutSize, desc.lutSize);

        return Environment(std::move(env), std::move(irradiance), std::move(prefiltered),
                           std::move(brdfLut), static_cast<f32>(desc.prefilterMips - 1u));
    }

    Environment::Environment(Cubemap env, Cubemap irradiance, Cubemap prefiltered, Texture brdfLut,
                             f32 prefilterMaxLod)
        : m_env(std::move(env)), m_irradiance(std::move(irradiance))
        , m_prefiltered(std::move(prefiltered)), m_brdfLut(std::move(brdfLut))
        , m_prefilterMaxLod(prefilterMaxLod) {}

}
}