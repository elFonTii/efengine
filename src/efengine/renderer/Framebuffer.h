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
            // Depth PROPIO: crea su renderbuffer y lo destruye al morir.
            Framebuffer(u32 width, u32 height);

            // Depth PRESTADO: se engancha al renderbuffer de otro y NO lo
            // destruye. Existe para el depth prepass -- el prepass del AO y el
            // forward tienen que escribir y testear contra el MISMO buffer, o el
            // forward no puede reusar lo que el prepass ya resolvio.
            //
            // El prestamo es cruzado con el ciclo de vida del dueno: si el dueno
            // se realoca (un resize crea un renderbuffer nuevo), este queda
            // apuntando a un handle muerto. Por eso Resize tiene una sobrecarga
            // que exige el handle nuevo, y por eso el orden de los dos resize en
            // Application::RenderScene no es negociable.
            Framebuffer(u32 width, u32 height, u32 externalDepthRbo);

            ~Framebuffer();
            Framebuffer(const Framebuffer&)            = delete;
            Framebuffer& operator=(const Framebuffer&) = delete;
            Framebuffer(Framebuffer&& other) noexcept;
            Framebuffer& operator=(Framebuffer&& other) noexcept;
            // A donde escribe un pase que dibuja a este FBO. Se copia por valor.
            RenderTarget    Target() const;
            void            Bind() const;   // azucar de Target().Bind()

            // Solo para framebuffers con depth propio. Sobre uno prestado
            // asertea: reconstruirlo sin el handle nuevo dejaria el prestamo
            // colgado en silencio, y un FBO incompleto no dibuja nada sin avisar.
            void            Resize(u32 width, u32 height);
            void            Resize(u32 width, u32 height, u32 externalDepthRbo);

            const Texture&  ColorTexture() const;
            u32             width() const;
            u32             height() const;

            // El renderbuffer de profundidad, para prestarselo a otro FBO.
            // Cambia en cada Resize: no se cachea.
            u32             depthRenderbuffer() const { return m_depthRbo; }
            bool            ownsDepth() const { return m_ownsDepth; }

        private:
            Framebuffer(u32 width, u32 height, u32 depthRbo, bool ownsDepth);

            u32     m_id        = 0;
            u32     m_depthRbo  = 0;
            bool    m_ownsDepth = true;
            Texture m_color;
            u32     m_width     = 0;
            u32     m_height    = 0;
    };
}
}