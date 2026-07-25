#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/Texture.h>

#include <optional>
#include <vector>

/* https://learnopengl.com/Advanced-OpenGL/Framebuffers */
/* https://wikis.khronos.org/opengl/Framebuffer */
namespace efengine {
namespace renderer {
    class Framebuffer {
        public:
            // FBO clásico: 1 color RGBA16F + depth renderbuffer (no muestreable).
            Framebuffer(u32 width, u32 height);
            // FBO con N color attachments (un internalFormat GL por attachment,
            // p.ej. {GL_RG16F, GL_R8}) y, si sampleableDepth, depth como Texture
            // D32F muestreable en vez de renderbuffer. Base del thin G-buffer.
            static std::optional<Framebuffer> CreateMRT(u32 width, u32 height,
                                                        const std::vector<u32>& colorFormats,
                                                        bool sampleableDepth);
            ~Framebuffer();
            Framebuffer(const Framebuffer&)            = delete;
            Framebuffer& operator=(const Framebuffer&) = delete;
            Framebuffer(Framebuffer&& other) noexcept;
            Framebuffer& operator=(Framebuffer&& other) noexcept;
            void            Bind() const;
            void            Unbind() const;
            void            Resize(u32 width, u32 height);
            const Texture&  ColorTexture(u32 index = 0) const;
            // Observador del depth muestreable; null si el depth es renderbuffer.
            const Texture*  DepthTexture() const;
            u32             colorCount() const;
            u32             width() const;
            u32             height() const;
        private:
            Framebuffer(u32 id, u32 depthRbo, std::vector<Texture> colors,
                        std::optional<Texture> depthTexture, std::vector<u32> colorFormats,
                        bool sampleableDepth, u32 width, u32 height);

            u32     m_id       = 0;
            u32     m_depthRbo = 0;   // depth renderbuffer (solo FBO clásico)
            std::vector<Texture>   m_colors;
            std::optional<Texture> m_depthTexture;
            std::vector<u32>       m_colorFormats;   // vacío = FBO clásico (para Resize)
            bool    m_sampleableDepth = false;
            u32     m_width    = 0;
            u32     m_height   = 0;
    };
}
}
