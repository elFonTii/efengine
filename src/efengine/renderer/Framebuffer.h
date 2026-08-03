#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/RenderTarget.h>

/* https://learnopengl.com/Advanced-OpenGL/Framebuffers */
/* https://wikis.khronos.org/opengl/Framebuffer */
namespace efengine {
namespace renderer {
    class Framebuffer {
        public:
            Framebuffer(u32 width, u32 height);
            ~Framebuffer();
            Framebuffer(const Framebuffer&)            = delete;
            Framebuffer& operator=(const Framebuffer&) = delete;
            Framebuffer(Framebuffer&& other) noexcept;
            Framebuffer& operator=(Framebuffer&& other) noexcept;
            // A donde escribe un pase que dibuja a este FBO. Se copia por valor.
            RenderTarget    Target() const;
            void            Bind() const;   // azucar de Target().Bind()
            void            Resize(u32 width, u32 height);
            const Texture&  ColorTexture() const;
            u32             width() const;
            u32             height() const;
        private:
            u32     m_id       = 0;
            u32     m_depthRbo = 0;
            Texture m_color;
            u32     m_width    = 0;
            u32     m_height   = 0;



    };
}
}