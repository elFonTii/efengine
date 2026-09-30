#include "efengine/renderer/PostTargets.h"

#include <efengine/core/Assert.h>

namespace efengine {
namespace renderer {

    PostTargets::PostTargets(u32 width, u32 height) : m_a(width, height), m_b(width, height) {}

    void PostTargets::BeginFrame(const Texture& sceneColor) {
        m_scene    = &sceneColor;
        m_external = null;
        m_state.Begin();
    }

    const Texture& PostTargets::Current() const {
        if (m_state.current == PingPong::kExternal) {
            EF_ASSERT(m_external != null, "PostTargets: externo sin framebuffer");
            return m_external->ColorTexture();
        }
        if (m_state.current < 0) {
            EF_ASSERT(m_scene != null, "PostTargets: Current() antes de BeginFrame");
            return *m_scene;
        }
        return At(static_cast<u32>(m_state.current)).ColorTexture();
    }

    const Framebuffer* PostTargets::CurrentFramebuffer() const {
        if (m_state.current == PingPong::kExternal) return m_external;
        return m_state.current < 0 ? null : &At(static_cast<u32>(m_state.current));
    }

    RenderTarget PostTargets::AcquireNext() { return At(m_state.Acquire()).Target(); }

    void PostTargets::Publish() { m_state.Publish(); }

    void PostTargets::PublishExternal(const Framebuffer& fb) {
        m_external = &fb;
        m_state.PublishExternal();
    }

    void PostTargets::Resize(u32 width, u32 height) {
        m_a.Resize(width, height);
        m_b.Resize(width, height);
    }

}
}
