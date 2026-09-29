#include "efengine/renderer/Framebuffer.h"
#include <efengine/core/Assert.h>
#include <efengine/core/Types.h>

#include <efecom/RHI.h>
#include <utility>

namespace efengine {
namespace renderer {

    Framebuffer::Framebuffer(u32 width, u32 height)
        : Framebuffer(width, height, std::optional<Texture>(Texture::CreateDepthAttachment(width, height)), 0u) {}

    Framebuffer::Framebuffer(u32 width, u32 height, u32 externalDepthTexture)
        : Framebuffer(width, height, std::nullopt, externalDepthTexture) {
        EF_ASSERT(externalDepthTexture != 0,
                  "Framebuffer: depth prestado invalido (handle 0)");
    }

    Framebuffer::Framebuffer(u32 width, u32 height, std::optional<Texture> ownedDepth, u32 depthId)
        : m_ownedDepth(std::move(ownedDepth))
        , m_depthId(m_ownedDepth ? m_ownedDepth->id() : depthId)
        , m_color(Texture::CreateColorAttachment(width, height))
        , m_width(width), m_height(height) {
        m_id = efecom::CreateFramebuffer();
        EF_ASSERT(m_id != 0, "Framebuffer::Framebuffer: No hay contexto GL");

        efecom::FramebufferColorTexture(m_id, m_color.id());
        efecom::FramebufferDepthTexture(m_id, m_depthId);

        EF_GPU_CHECK(efecom::FramebufferComplete(m_id), "Framebuffer incompleto");
    }

    Framebuffer::~Framebuffer() {
        if (m_id != 0) efecom::DestroyFramebuffer(m_id);
    }

    Framebuffer::Framebuffer(Framebuffer&& other) noexcept
        : m_id(std::exchange(other.m_id, 0))
        , m_ownedDepth(std::move(other.m_ownedDepth))
        , m_depthId(std::exchange(other.m_depthId, 0))
        , m_color(std::move(other.m_color))
        , m_width(std::exchange(other.m_width, 0))
        , m_height(std::exchange(other.m_height, 0)) {
        other.m_ownedDepth.reset();
    }

    Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
        if (this != &other) {
            if (m_id != 0) efecom::DestroyFramebuffer(m_id);

            m_id         = std::exchange(other.m_id, 0);
            m_ownedDepth = std::move(other.m_ownedDepth);
            other.m_ownedDepth.reset();
            m_depthId    = std::exchange(other.m_depthId, 0);
            m_color      = std::move(other.m_color);
            m_width      = std::exchange(other.m_width, 0);
            m_height     = std::exchange(other.m_height, 0);
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

    const Texture& Framebuffer::depthTexture() const {
        EF_ASSERT(m_ownedDepth.has_value(), "Framebuffer::depthTexture: el depth es prestado");
        return *m_ownedDepth;
    }

    void Framebuffer::Resize(u32 width, u32 height) {
        EF_ASSERT(ownsDepth(),
                  "Framebuffer::Resize: este FBO tiene el depth prestado; usa la "
                  "sobrecarga que recibe la textura nueva del dueno");
        if (width == m_width && height == m_height) return;
        *this = Framebuffer(width, height);
    }

    void Framebuffer::Resize(u32 width, u32 height, u32 externalDepthTexture) {
        if (width == m_width && height == m_height && externalDepthTexture == m_depthId) return;
        *this = Framebuffer(width, height, externalDepthTexture);
    }
}
}
