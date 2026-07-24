#include "efengine/renderer/Environment.h"
#include <efengine/renderer/Texture.h>
#include <efengine/core/Log.h>
#include <efengine/core/Assert.h>

#include <glad/gl.h>
#include <utility>



namespace efengine {
namespace renderer {

    std::optional<Environment> Environment::Create(const char* hdrPath,
                                                   const Shader& equirectToCube,
                                                   u32 faceSize) {
        auto hdr = Texture::CreateHDR(hdrPath);
        if (!hdr) {
            EF_LOG_ERROR("Environment::Create: no se pudo cargar el HDR '%s'", hdrPath);
            return std::nullopt;
        }

        Cubemap env = Cubemap::Create(faceSize, GL_RGBA16F, mipLevels(faceSize));

        equirectToCube.Bind();
        hdr->Bind(0);
        env.BindImage(0, 0, GL_WRITE_ONLY, GL_RGBA16F);

        const u32 groups = (faceSize + 7u) / 8u;
        glDispatchCompute(groups, groups, 6);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        env.GenerateMips();

        EF_LOG_INFO("Environment: IBL precomputado desde '%s' (%ux%u/cara)", hdrPath, faceSize, faceSize);

        return Environment(std::move(env));
    }

    Environment::Environment(Cubemap env) : m_env(std::move(env)) {}

}
}