#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/RenderTarget.h>

namespace efengine {
namespace renderer {

    // FBO solo profundidad + textura de profundidad
    class ShadowMap {
        public:
            explicit ShadowMap(u32 resolution = 2048);
            ~ShadowMap();
            ShadowMap(const ShadowMap&)            = delete;
            ShadowMap& operator=(const ShadowMap&) = delete;
            ShadowMap(ShadowMap&& other) noexcept;
            ShadowMap& operator=(ShadowMap&& other) noexcept;

            RenderTarget   Target() const;   // el FBO + su resolucion como viewport
            void           Bind() const;     // azucar de Target().Bind()
            const Texture& DepthTexture() const { return m_depth; }
            u32 resolution() const { return m_resolution; }

        private:
            u32     m_id = 0;
            Texture m_depth;
            u32     m_resolution = 0;
    };

}
}
