#include "efengine/renderer/Framebuffer.h"
#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>
#include <efengine/core/Types.h>

#include <glad/gl.h>
#include <utility>

namespace efengine {
namespace renderer {

    Framebuffer::Framebuffer(u32 width, u32 height) : m_width(width), m_height(height) {
        // secuencia de fbo:  -1 pedir id -2 activarlo -3 enganchar color 4- enganchar depthbuffer -5 desactivar
        m_colors.push_back(Texture::CreateColorAttachment(width, height));

        glGenFramebuffers(1, &m_id); // -1
        EF_ASSERT(m_id != 0, "Framebuffer::Framebuffer: No hay contexto GL");

        glBindFramebuffer(GL_FRAMEBUFFER, m_id); // -2
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colors[0].id(), 0); // -3

        glGenRenderbuffers(1, &m_depthRbo);
        glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, (GLsizei)width, (GLsizei)height); // reserva buffer 24bits
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRbo);  // -4

        EF_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer incompleto");

        glBindFramebuffer(GL_FRAMEBUFFER, 0); // -5
    }

    std::optional<Framebuffer> Framebuffer::CreateMRT(u32 width, u32 height,
                                                      const std::vector<u32>& colorFormats,
                                                      bool sampleableDepth) {
        EF_ASSERT(width > 0 && height > 0, "Framebuffer::CreateMRT: dimensiones invalidas");
        EF_ASSERT(!colorFormats.empty(), "Framebuffer::CreateMRT: hace falta al menos un color attachment");

        u32 id = 0;
        glGenFramebuffers(1, &id);
        EF_ASSERT(id != 0, "Framebuffer::CreateMRT: No hay contexto GL");
        glBindFramebuffer(GL_FRAMEBUFFER, id);

        std::vector<Texture> colors;
        colors.reserve(colorFormats.size());
        std::vector<GLenum> drawBuffers;
        drawBuffers.reserve(colorFormats.size());

        for (usize i = 0; i < colorFormats.size(); ++i) {
            Texture color = Texture::CreateEmpty(width, height, colorFormats[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (GLenum)i,
                                   GL_TEXTURE_2D, color.id(), 0);
            drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + (GLenum)i);
            colors.push_back(std::move(color));
        }
        glDrawBuffers((GLsizei)drawBuffers.size(), drawBuffers.data());

        std::optional<Texture> depthTexture;
        u32 depthRbo = 0;
        if (sampleableDepth) {
            depthTexture = Texture::CreateDepthAttachment(width, height);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                                   depthTexture->id(), 0);
        } else {
            glGenRenderbuffers(1, &depthRbo);
            glBindRenderbuffer(GL_RENDERBUFFER, depthRbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, (GLsizei)width, (GLsizei)height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRbo);
        }

        // Un formato no renderizable es un fallo recuperable → nullopt (las
        // Texture ya creadas se liberan solas por RAII).
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            EF_LOG_ERROR("Framebuffer::CreateMRT: framebuffer incompleto (%u attachments)",
                         (u32)colorFormats.size());
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            if (depthRbo != 0) glDeleteRenderbuffers(1, &depthRbo);
            glDeleteFramebuffers(1, &id);
            return std::nullopt;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return Framebuffer(id, depthRbo, std::move(colors), std::move(depthTexture),
                           colorFormats, sampleableDepth, width, height);
    }

    Framebuffer::Framebuffer(u32 id, u32 depthRbo, std::vector<Texture> colors,
                             std::optional<Texture> depthTexture, std::vector<u32> colorFormats,
                             bool sampleableDepth, u32 width, u32 height)
        : m_id(id)
        , m_depthRbo(depthRbo)
        , m_colors(std::move(colors))
        , m_depthTexture(std::move(depthTexture))
        , m_colorFormats(std::move(colorFormats))
        , m_sampleableDepth(sampleableDepth)
        , m_width(width)
        , m_height(height) {}

    Framebuffer::~Framebuffer() {
        if (m_depthRbo != 0) glDeleteRenderbuffers(1, &m_depthRbo);
        if (m_id != 0)       glDeleteFramebuffers(1, &m_id);
    }

    Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : m_id(std::exchange(other.m_id, 0))
    , m_depthRbo(std::exchange(other.m_depthRbo, 0))
    , m_colors(std::move(other.m_colors))
    , m_depthTexture(std::move(other.m_depthTexture))
    , m_colorFormats(std::move(other.m_colorFormats))
    , m_sampleableDepth(std::exchange(other.m_sampleableDepth, false))
    , m_width(std::exchange(other.m_width, 0))
    , m_height(std::exchange(other.m_height, 0)) {}

    Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
        if(this != &other) {
            if (m_depthRbo != 0) glDeleteRenderbuffers(1, &m_depthRbo);
            if (m_id != 0)       glDeleteFramebuffers(1, &m_id);

            m_id = std::exchange(other.m_id, 0);
            m_depthRbo = std::exchange(other.m_depthRbo, 0);
            m_colors = std::move(other.m_colors);
            m_depthTexture = std::move(other.m_depthTexture);
            m_colorFormats = std::move(other.m_colorFormats);
            m_sampleableDepth = std::exchange(other.m_sampleableDepth, false);
            m_width = std::exchange(other.m_width, 0);
            m_height = std::exchange(other.m_height, 0);
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

    const Texture& Framebuffer::ColorTexture(u32 index) const {
        EF_ASSERT(index < m_colors.size(), "Framebuffer::ColorTexture: indice fuera de rango");
        return m_colors[index];
    }

    const Texture* Framebuffer::DepthTexture() const {
        return m_depthTexture ? &(*m_depthTexture) : null;
    }

    u32 Framebuffer::colorCount() const {
        return (u32)m_colors.size();
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
        if (m_colorFormats.empty()) {
            *this = Framebuffer(width, height);
        } else {
            std::optional<Framebuffer> recreated = CreateMRT(width, height, m_colorFormats, m_sampleableDepth);
            // Ya fue completo con estos formatos una vez; si ahora falla es un
            // error de programa, no un fallo recuperable.
            EF_ASSERT(recreated.has_value(), "Framebuffer::Resize: no se pudo recrear el MRT");
            *this = std::move(*recreated);
        }
    }
}
}
