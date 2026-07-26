#include "efengine/renderer/Environment.h"
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
            || shaders.brdfLut == null) {
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
        env.Bind(0);                                    // entorno como samplerCube (unit 0)
        irradiance.BindImage(0, 0, efecom::ImageAccess::WriteOnly, efecom::TextureFormat::RGBA16F);

        const u32 iGroups = (desc.irradianceSize + 7u) / 8u;
        efecom::DispatchCompute(iGroups, iGroups, 6);
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

        EF_LOG_INFO("Environment: IBL precomputado desde '%s' (env %ux%u, irradiancia %ux%u, LUT %ux%u)",
                    desc.hdrPath, desc.faceSize, desc.faceSize,
                    desc.irradianceSize, desc.irradianceSize, desc.lutSize, desc.lutSize);

        return Environment(std::move(env), std::move(irradiance), std::move(brdfLut));
    }

    Environment::Environment(Cubemap env, Cubemap irradiance, Texture brdfLut)
        : m_env(std::move(env)), m_irradiance(std::move(irradiance)), m_brdfLut(std::move(brdfLut)) {}

}
}