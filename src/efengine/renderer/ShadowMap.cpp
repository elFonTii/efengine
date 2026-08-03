#include "efengine/renderer/ShadowMap.h"

#include <efecom/RHI.h>
#include <utility>

#include <efengine/core/Assert.h>

namespace efengine {
namespace renderer {

    ShadowMap::ShadowMap(u32 resolution)
        : m_depth(Texture::CreateDepthAttachment(resolution, resolution))
        , m_resolution(resolution) {
        m_id = efecom::CreateFramebuffer();
        EF_ASSERT(m_id != 0, "ShadowMap::ShadowMap: No hay contexto GL");

        efecom::FramebufferDepthTexture(m_id, m_depth.id());
        efecom::FramebufferDisableColor(m_id);

        EF_GPU_CHECK(efecom::FramebufferComplete(m_id),
                  "ShadowMap: framebuffer incompleto");
    }

    ShadowMap::~ShadowMap() {
        if (m_id != 0) efecom::DestroyFramebuffer(m_id);
    }

    ShadowMap::ShadowMap(ShadowMap&& other) noexcept
        : m_id(std::exchange(other.m_id, 0))
        , m_depth(std::move(other.m_depth))
        , m_resolution(std::exchange(other.m_resolution, 0)) {}

    ShadowMap& ShadowMap::operator=(ShadowMap&& other) noexcept {
        if (this != &other) {
            if (m_id != 0) efecom::DestroyFramebuffer(m_id);
            m_id         = std::exchange(other.m_id, 0);
            m_depth      = std::move(other.m_depth);
            m_resolution = std::exchange(other.m_resolution, 0);
        }
        return *this;
    }

    RenderTarget ShadowMap::Target() const {
        return RenderTarget(m_id, m_resolution, m_resolution);
    }

    void ShadowMap::Bind() const { Target().Bind(); }

}
}
