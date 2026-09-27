#include "efengine/renderer/CascadedShadowMap.h"

#include <efecom/RHI.h>
#include <utility>

#include <efengine/core/Assert.h>

namespace efengine {
namespace renderer {

    CascadedShadowMap::CascadedShadowMap(u32 resolution, u32 layers)
        : m_resolution(resolution), m_layers(layers) {
        m_texture = efecom::CreateDepthTexture2DArray(resolution, layers);
        EF_ASSERT(m_texture != 0, "CascadedShadowMap: No hay contexto GL");

        m_fbo = efecom::CreateFramebuffer();
        EF_ASSERT(m_fbo != 0, "CascadedShadowMap: No hay contexto GL");

        efecom::FramebufferDisableColor(m_fbo);
        // Completo se chequea con la capa 0 adjunta: un FBO sin ningun attachment
        // no lo esta, y no diria nada sobre si el array sirve.
        efecom::FramebufferDepthTextureLayer(m_fbo, m_texture, 0u);
        EF_GPU_CHECK(efecom::FramebufferComplete(m_fbo),
                     "CascadedShadowMap: framebuffer incompleto");
    }

    CascadedShadowMap::~CascadedShadowMap() {
        if (m_fbo != 0)     efecom::DestroyFramebuffer(m_fbo);
        if (m_texture != 0) efecom::DestroyTexture(m_texture);
    }

    CascadedShadowMap::CascadedShadowMap(CascadedShadowMap&& other) noexcept
        : m_fbo(std::exchange(other.m_fbo, 0))
        , m_texture(std::exchange(other.m_texture, 0))
        , m_resolution(std::exchange(other.m_resolution, 0))
        , m_layers(std::exchange(other.m_layers, 0)) {}

    CascadedShadowMap& CascadedShadowMap::operator=(CascadedShadowMap&& other) noexcept {
        if (this != &other) {
            if (m_fbo != 0)     efecom::DestroyFramebuffer(m_fbo);
            if (m_texture != 0) efecom::DestroyTexture(m_texture);
            m_fbo        = std::exchange(other.m_fbo, 0);
            m_texture    = std::exchange(other.m_texture, 0);
            m_resolution = std::exchange(other.m_resolution, 0);
            m_layers     = std::exchange(other.m_layers, 0);
        }
        return *this;
    }

    void CascadedShadowMap::BindLayer(u32 layer) const {
        EF_ASSERT(layer < m_layers, "CascadedShadowMap::BindLayer: capa fuera de rango");
        efecom::FramebufferDepthTextureLayer(m_fbo, m_texture, layer);
        efecom::BindRenderTarget(m_fbo, m_resolution, m_resolution);
    }

    void CascadedShadowMap::BindTexture(u32 unit) const {
        efecom::BindTextureUnit(m_texture, unit);
    }

}
}
