#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/RenderTarget.h>

#include <efecom/RHI.h>

#include <initializer_list>
#include <optional>
#include <vector>

/* https://learnopengl.com/Advanced-OpenGL/Framebuffers */
/* https://wikis.khronos.org/opengl/Framebuffer */
namespace efengine {
namespace renderer {
    class Framebuffer {
        public:
            // Depth PROPIO: textura D32F, muestreable.
            Framebuffer(u32 width, u32 height);

            // Depth PRESTADO: se engancha a la textura de otro y NO la destruye.
            // El prepass y el forward escriben y testean contra el MISMO depth.
            // Si el dueno se realoca, el handle viejo muere: por eso Resize exige
            // el nuevo y el orden de los resize en Application no es negociable.
            Framebuffer(u32 width, u32 height, u32 externalDepthTexture);

            // Depth PRESTADO y varios colores (MRT). colors[0] es ColorTexture().
            Framebuffer(u32 width, u32 height, u32 externalDepthTexture,
                        std::initializer_list<efecom::TextureFormat> colors);

            ~Framebuffer();
            Framebuffer(const Framebuffer&)            = delete;
            Framebuffer& operator=(const Framebuffer&) = delete;
            Framebuffer(Framebuffer&& other) noexcept;
            Framebuffer& operator=(Framebuffer&& other) noexcept;

            RenderTarget    Target() const;
            void            Bind() const;

            // Solo con depth propio; sobre uno prestado asertea.
            void            Resize(u32 width, u32 height);
            void            Resize(u32 width, u32 height, u32 externalDepthTexture);

            const Texture&  ColorTexture() const;
            const Texture&  ColorTexture(u32 index) const;
            u32             colorCount() const { return static_cast<u32>(m_formats.size()); }
            u32             width() const;
            u32             height() const;
            u32             id() const { return m_id; }

            // Cambia en cada Resize: no se cachea.
            u32             depthTextureId() const { return m_depthId; }
            // Solo con depth propio.
            const Texture&  depthTexture() const;
            bool            ownsDepth() const { return m_ownedDepth.has_value(); }

        private:
            Framebuffer(u32 width, u32 height, std::optional<Texture> ownedDepth, u32 depthId,
                        std::vector<efecom::TextureFormat> formats);

            u32                    m_id = 0;
            std::optional<Texture> m_ownedDepth;
            u32                    m_depthId = 0;
            Texture                m_color;
            u32                    m_width  = 0;
            u32                    m_height = 0;
            std::vector<efecom::TextureFormat> m_formats;
            std::vector<Texture>               m_extraColors;
    };
}
}