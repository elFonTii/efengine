#include "efengine/renderer/Framebuffer.h"
#include <efengine/core/Assert.h>
#include <efengine/core/Types.h>

#include <efecom/RHI.h>
#include <utility>

namespace efengine {
namespace renderer {

    Framebuffer::Framebuffer(u32 width, u32 height)
        : Framebuffer(width, height, std::optional<Texture>(Texture::CreateDepthAttachment(width, height)), 0u,
                      std::vector<efecom::TextureFormat>{ efecom::TextureFormat::RGBA16F }) {}

    Framebuffer::Framebuffer(u32 width, u32 height, u32 externalDepthTexture)
        : Framebuffer(width, height, std::nullopt, externalDepthTexture,
                      std::vector<efecom::TextureFormat>{ efecom::TextureFormat::RGBA16F }) {
        EF_ASSERT(externalDepthTexture != 0,
                  "Framebuffer: depth prestado invalido (handle 0)");
    }

    Framebuffer::Framebuffer(u32 width, u32 height, u32 externalDepthTexture,
                             std::initializer_list<efecom::TextureFormat> colors)
        : Framebuffer(width, height, std::nullopt, externalDepthTexture,
                      std::vector<efecom::TextureFormat>(colors)) {
        EF_ASSERT(externalDepthTexture != 0,
                  "Framebuffer: depth prestado invalido (handle 0)");
    }

    Framebuffer::Framebuffer(u32 width, u32 height, std::optional<Texture> ownedDepth, u32 depthId,
                             std::vector<efecom::TextureFormat> formats)
        : m_ownedDepth(std::move(ownedDepth))
        , m_depthId(m_ownedDepth ? m_ownedDepth->id() : depthId)
        , m_color(Texture::CreateColorAttachment(width, height,
                      formats.empty() ? efecom::TextureFormat::RGBA16F : formats.front()))
        , m_width(width), m_height(height)
        , m_formats(std::move(formats)) {
        EF_ASSERT(!m_formats.empty(), "Framebuffer: sin formatos de color");
        m_id = efecom::CreateFramebuffer();
        EF_ASSERT(m_id != 0, "Framebuffer::Framebuffer: No hay contexto GL");

        efecom::FramebufferColorTexture(m_id, m_color.id());
        for (u32 i = 1u; i < m_formats.size(); ++i) {
            m_extraColors.push_back(Texture::CreateColorAttachment(width, height, m_formats[i]));
            efecom::FramebufferColorTexture(m_id, i, m_extraColors.back().id());
        }
        if (m_formats.size() > 1u) efecom::FramebufferDrawBuffers(m_id, static_cast<u32>(m_formats.size()));
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
        , m_height(std::exchange(other.m_height, 0))
        , m_formats(std::move(other.m_formats))
        , m_extraColors(std::move(other.m_extraColors)) {
        other.m_ownedDepth.reset();
    }

    Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
        if (this != &other) {
            if (m_id != 0) efecom::DestroyFramebuffer(m_id);

            m_id          = std::exchange(other.m_id, 0);
            m_ownedDepth  = std::move(other.m_ownedDepth);
            other.m_ownedDepth.reset();
            m_depthId     = std::exchange(other.m_depthId, 0);
            m_color       = std::move(other.m_color);
            m_width       = std::exchange(other.m_width, 0);
            m_height      = std::exchange(other.m_height, 0);
            m_formats     = std::move(other.m_formats);
            m_extraColors = std::move(other.m_extraColors);
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

    const Texture& Framebuffer::ColorTexture(u32 index) const {
        if (index == 0u) return m_color;
        EF_ASSERT(index - 1u < m_extraColors.size(), "Framebuffer::ColorTexture: indice fuera de rango");
        return m_extraColors[index - 1u];
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
        *this = Framebuffer(width, height, std::optional<Texture>(Texture::CreateDepthAttachment(width, height)),
                            0u, m_formats);
    }

    void Framebuffer::Resize(u32 width, u32 height, u32 externalDepthTexture) {
        if (width == m_width && height == m_height && externalDepthTexture == m_depthId) return;
        *this = Framebuffer(width, height, std::nullopt, externalDepthTexture, m_formats);
    }
}
}
