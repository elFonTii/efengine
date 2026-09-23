#pragma once
#include <efengine/core/Types.h>

namespace efengine {
namespace renderer {

    // Un FBO sin color y un array de profundidad: una capa por cascada.
    //
    // No reusa Texture porque Texture es siempre 2D (guarda width/height y bindea
    // como GL_TEXTURE_2D). El id del array se bindea con BindTextureUnit, que
    // sirve para cualquier tipo.
    class CascadedShadowMap {
        public:
            CascadedShadowMap(u32 resolution, u32 layers);
            ~CascadedShadowMap();
            CascadedShadowMap(const CascadedShadowMap&)            = delete;
            CascadedShadowMap& operator=(const CascadedShadowMap&) = delete;
            CascadedShadowMap(CascadedShadowMap&& other) noexcept;
            CascadedShadowMap& operator=(CascadedShadowMap&& other) noexcept;

            // Adjunta la capa y bindea el FBO con el viewport ya puesto.
            void BindLayer(u32 layer) const;
            void BindTexture(u32 unit) const;

            u32 resolution() const { return m_resolution; }
            u32 layers()     const { return m_layers; }

        private:
            u32 m_fbo        = 0;
            u32 m_texture    = 0;
            u32 m_resolution = 0;
            u32 m_layers     = 0;
    };

}
}
