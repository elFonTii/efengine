#include "efengine/renderer/Framebuffer.h"
#include <efengine/core/Assert.h>
#include <efengine/core/Types.h>

#include <glad/gl.h>
#include <utility>

namespace efengine {
namespace renderer {

    Framebuffer::Framebuffer(u32 width, u32 height) : m_color(Texture::CreateColorAttachment(width, height)), m_width(width), m_height(height) {
        // secuencia de fbo:  -1 pedir id -2 activarlo -3 enganchar color 4- enganchar depthbuffer -5 desactivar
        
        glGenFramebuffers(1, &m_id); // -1
        EF_ASSERT(m_id != 0, "Framebuffer::Framebuffer: No hay contexto GL");

        glBindFramebuffer(GL_FRAMEBUFFER, m_id); // -2
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_color.id(), 0); // -3

        glGenRenderbuffers(1, &m_depthRbo);
        glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, (GLsizei)width, (GLsizei)height); // reserva buffer 24bits
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRbo);  // -4

        EF_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer incompleto");

        glBindFramebuffer(GL_FRAMEBUFFER, 0); // -5
    }

    Framebuffer::~Framebuffer() {
        if (m_depthRbo != 0) glDeleteRenderbuffers(1, &m_depthRbo);
        if (m_id != 0)       glDeleteFramebuffers(1, &m_id);
    }

    Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : m_id(std::exchange(other.m_id, 0))
    , m_depthRbo(std::exchange(other.m_depthRbo, 0))
    , m_color(std::move(other.m_color))
    , m_width(std::exchange(other.m_width, 0))
    , m_height(std::exchange(other.m_height, 0)) {}

    Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
        if(this != &other) {
            if (m_depthRbo != 0) glDeleteRenderbuffers(1, &m_depthRbo);
            if (m_id != 0)       glDeleteFramebuffers(1, &m_id);

            m_id = std::exchange(other.m_id, 0);
            m_depthRbo = std::exchange(other.m_depthRbo, 0);
            m_color = std::move(other.m_color);
            m_width = std::move(other.m_width);
            m_height = std::move(other.m_height);
        }
        return *this;
    }

    void Framebuffer::Bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER,  m_id);
        glViewport(0, 0, (GLsizei)m_width, (GLsizei)m_height);
    }

    void Framebuffer::Unbind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    const Texture& Framebuffer::ColorTexture() const {
        return m_color;
    }

    u32 Framebuffer::width() const {
        return m_width;
    };

    u32 Framebuffer::height() const {
        return m_height;
    };

    void Framebuffer::Resize(u32 width, u32 height) {
        if(width == m_width && height == m_height) return; // nada que redimensionar

        // desreferencia, creo un framebuffer nuevo y copio
        *this = Framebuffer(width, height);
    }
}
}