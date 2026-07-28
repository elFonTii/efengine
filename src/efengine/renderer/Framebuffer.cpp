#include "efengine/renderer/Framebuffer.h"
#include <efengine/core/Assert.h>
#include <efengine/core/Types.h>

#include <efecom/RHI.h>
#include <utility>

namespace efengine {
namespace renderer {

    Framebuffer::Framebuffer(u32 width, u32 height) : m_color(Texture::CreateColorAttachment(width, height)), m_width(width), m_height(height) {
        // secuencia de fbo:  -1 pedir id -2 enganchar color -3 enganchar depthbuffer -4 verificar completo

        m_id = efecom::CreateFramebuffer(); // -1
        EF_ASSERT(m_id != 0, "Framebuffer::Framebuffer: No hay contexto GL");

        efecom::FramebufferColorTexture(m_id, m_color.id()); // -2

        m_depthRbo = efecom::CreateDepthRenderbuffer(width, height); // reserva buffer 24bits
        efecom::FramebufferDepthRenderbuffer(m_id, m_depthRbo); // -3

        EF_ASSERT(efecom::FramebufferComplete(m_id), "Framebuffer incompleto"); // -4
    }

    Framebuffer::~Framebuffer() {
        if (m_depthRbo != 0) efecom::DestroyRenderbuffer(m_depthRbo);
        if (m_id != 0)       efecom::DestroyFramebuffer(m_id);
    }

    Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : m_id(std::exchange(other.m_id, 0))
    , m_depthRbo(std::exchange(other.m_depthRbo, 0))
    , m_color(std::move(other.m_color))
    , m_width(std::exchange(other.m_width, 0))
    , m_height(std::exchange(other.m_height, 0)) {}

    Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
        if(this != &other) {
            if (m_depthRbo != 0) efecom::DestroyRenderbuffer(m_depthRbo);
            if (m_id != 0)       efecom::DestroyFramebuffer(m_id);

            m_id = std::exchange(other.m_id, 0);
            m_depthRbo = std::exchange(other.m_depthRbo, 0);
            m_color = std::move(other.m_color);
            m_width = std::move(other.m_width);
            m_height = std::move(other.m_height);
        }
        return *this;
    }

    RenderTarget Framebuffer::Target() const {
        return RenderTarget(m_id, m_width, m_height);
    }

    void Framebuffer::Bind() const { Target().Bind(); }

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
