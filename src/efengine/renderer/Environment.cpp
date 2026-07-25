#include "efengine/renderer/Environment.h"
#include <efengine/renderer/Texture.h>
#include <efengine/core/Log.h>
#include <efengine/core/Assert.h>

#include <efecom/RHI.h>
#include <utility>



namespace efengine {
namespace renderer {

    std::optional<Environment> Environment::Create(const char* hdrPath,
                                                   const Shader& equirectToCube,
                                                   const Shader& irradianceConvolve,
                                                   u32 faceSize,
                                                   u32 irradianceSize) {
        auto hdr = Texture::CreateHDR(hdrPath);
        if (!hdr) {
            EF_LOG_ERROR("Environment::Create: no se pudo cargar el HDR '%s'", hdrPath);
            return std::nullopt;
        }

        Cubemap env = Cubemap::Create(faceSize, efecom::TextureFormat::RGBA16F, mipLevels(faceSize));

        equirectToCube.Bind();
        hdr->Bind(0);
        env.BindImage(0, 0, efecom::ImageAccess::WriteOnly, efecom::TextureFormat::RGBA16F);

        const u32 groups = (faceSize + 7u) / 8u;
        efecom::DispatchCompute(groups, groups, 6);

        efecom::IssueMemoryBarrier(efecom::Barrier::ShaderImageAccess | efecom::Barrier::TextureFetch);

        env.GenerateMips();

        // --- Segunda pasada: convolución difusa del entorno a irradiancia ---
        Cubemap irradiance = Cubemap::Create(irradianceSize, efecom::TextureFormat::RGBA16F, 1);

        irradianceConvolve.Bind();
        env.Bind(0);                                    // entorno como samplerCube (unit 0)
        irradiance.BindImage(0, 0, efecom::ImageAccess::WriteOnly, efecom::TextureFormat::RGBA16F);

        const u32 iGroups = (irradianceSize + 7u) / 8u;
        efecom::DispatchCompute(iGroups, iGroups, 6);
        efecom::IssueMemoryBarrier(efecom::Barrier::ShaderImageAccess | efecom::Barrier::TextureFetch);

        EF_LOG_INFO("Environment: IBL precomputado desde '%s' (env %ux%u, irradiancia %ux%u)",
                    hdrPath, faceSize, faceSize, irradianceSize, irradianceSize);

        return Environment(std::move(env), std::move(irradiance));
    }

    Environment::Environment(Cubemap env, Cubemap irradiance)
        : m_env(std::move(env)), m_irradiance(std::move(irradiance)) {}

}
}