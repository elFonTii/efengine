#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/Texture.h>

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
            void            Bind() const;
            void            Unbind() const;
            void            Resize(u32 width, u32 height);
            const Texture&  ColorTexture() const;
            u32             width() const;
            u32             height() const;
        private:
            u32     m_id       = 0; 
            u32     m_depthRbo = 0;   // depth renderbuffer
            Texture m_color;
            u32     m_width    = 0;
            u32     m_height   = 0;



    };
}
}