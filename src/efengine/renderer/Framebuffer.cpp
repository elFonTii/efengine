#include "efengine/renderer/Framebuffer.h"
#include <efengine/core/Assert.h>
#include <efengine/core/Types.h>

#include <efecom/RHI.h>
#include <utility>

namespace efengine {
namespace renderer {

    Framebuffer::Framebuffer(u32 width, u32 height)
        : Framebuffer(width, height, efecom::CreateDepthRenderbuffer(width, height), true) {}

    Framebuffer::Framebuffer(u32 width, u32 height, u32 externalDepthRbo)
        : Framebuffer(width, height, externalDepthRbo, false) {
        // Un handle en cero deja el FBO sin profundidad: dibujar contra el
        // saldria sin depth test, que es una diferencia visual grande y muda.
        EF_ASSERT(externalDepthRbo != 0,
                  "Framebuffer: depth prestado invalido (handle 0)");
    }

    Framebuffer::Framebuffer(u32 width, u32 height, u32 depthRbo, bool ownsDepth)
        : m_depthRbo(depthRbo), m_ownsDepth(ownsDepth)
        , m_color(Texture::CreateColorAttachment(width, height))
        , m_width(width), m_height(height) {
        m_id = efecom::CreateFramebuffer();
        EF_ASSERT(m_id != 0, "Framebuffer::Framebuffer: No hay contexto GL");

        efecom::FramebufferColorTexture(m_id, m_color.id());
        efecom::FramebufferDepthRenderbuffer(m_id, m_depthRbo);

        EF_GPU_CHECK(efecom::FramebufferComplete(m_id), "Framebuffer incompleto");
    }

    Framebuffer::~Framebuffer() {
        // Solo el dueno destruye el renderbuffer. Un prestado que lo destruyera
        // dejaria al dueno con un attachment muerto.
        if (m_ownsDepth && m_depthRbo != 0) efecom::DestroyRenderbuffer(m_depthRbo);
        if (m_id != 0)                      efecom::DestroyFramebuffer(m_id);
    }

    Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : m_id(std::exchange(other.m_id, 0))
    , m_depthRbo(std::exchange(other.m_depthRbo, 0))
    , m_ownsDepth(std::exchange(other.m_ownsDepth, true))
    , m_color(std::move(other.m_color))
    , m_width(std::exchange(other.m_width, 0))
    , m_height(std::exchange(other.m_height, 0)) {}

    Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
        if(this != &other) {
            if (m_ownsDepth && m_depthRbo != 0) efecom::DestroyRenderbuffer(m_depthRbo);
            if (m_id != 0)                      efecom::DestroyFramebuffer(m_id);

            m_id        = std::exchange(other.m_id, 0);
            m_depthRbo  = std::exchange(other.m_depthRbo, 0);
            m_ownsDepth = std::exchange(other.m_ownsDepth, true);
            m_color     = std::move(other.m_color);
            m_width     = std::exchange(other.m_width, 0);
            m_height    = std::exchange(other.m_height, 0);
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
        // Sobre un prestado no hay forma de acertar: el renderbuffer nuevo lo
        // crea el DUENO, y este objeto no sabe cual es. Reconstruirlo con uno
        // propio romperia el prepass en silencio -- el forward testearia contra
        // una profundidad que nadie escribio y no dibujaria nada.
        EF_ASSERT(m_ownsDepth,
                  "Framebuffer::Resize: este FBO tiene el depth prestado; usa la "
                  "sobrecarga que recibe el renderbuffer nuevo del dueno");

        if(width == m_width && height == m_height) return;

        *this = Framebuffer(width, height);
    }

    void Framebuffer::Resize(u32 width, u32 height, u32 externalDepthRbo) {
        // El handle tambien es parte del estado: el dueno pudo haberse realocado
        // sin cambiar de tamano, y quedarse con el viejo seria un attachment
        // muerto.
        if (width == m_width && height == m_height && externalDepthRbo == m_depthRbo) return;

        *this = Framebuffer(width, height, externalDepthRbo);
    }
}
}
