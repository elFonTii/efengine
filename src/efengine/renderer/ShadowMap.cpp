#include "efengine/renderer/ShadowMap.h"

#include <glad/gl.h>
#include <utility>

#include <efengine/core/Assert.h>

namespace efengine {
namespace renderer {

    ShadowMap::ShadowMap(u32 resolution)
        : m_depth(Texture::CreateDepthAttachment(resolution, resolution))
        , m_resolution(resolution) {
        glGenFramebuffers(1, &m_id);
        EF_ASSERT(m_id != 0, "ShadowMap::ShadowMap: No hay contexto GL");

        glBindFramebuffer(GL_FRAMEBUFFER, m_id);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depth.id(), 0);
        // Sin color attachment: no dibujamos ni leemos color.
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        EF_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                  "ShadowMap: framebuffer incompleto");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    ShadowMap::~ShadowMap() {
        if (m_id != 0) glDeleteFramebuffers(1, &m_id);
    }

    ShadowMap::ShadowMap(ShadowMap&& other) noexcept
        : m_id(std::exchange(other.m_id, 0))
        , m_depth(std::move(other.m_depth))
        , m_resolution(std::exchange(other.m_resolution, 0)) {}

    ShadowMap& ShadowMap::operator=(ShadowMap&& other) noexcept {
        if (this != &other) {
            if (m_id != 0) glDeleteFramebuffers(1, &m_id);
            m_id         = std::exchange(other.m_id, 0);
            m_depth      = std::move(other.m_depth);
            m_resolution = std::exchange(other.m_resolution, 0);
        }
        return *this;
    }

    void ShadowMap::Bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, m_id);
        glViewport(0, 0, (GLsizei)m_resolution, (GLsizei)m_resolution);
    }

    void ShadowMap::Unbind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

}
}
