#include "efengine/renderer/RenderTarget.h"

#include <efecom/RHI.h>

namespace efengine {
namespace renderer {

    RenderTarget RenderTarget::Present() {
        efecom::u32 w = 0u, h = 0u;
        efecom::GetPresentExtent(w, h);
        return RenderTarget(efecom::GetPresentTarget(),
                            static_cast<u32>(w), static_cast<u32>(h));
    }

    void RenderTarget::Bind() const {
        efecom::BindRenderTarget(m_handle, m_width, m_height);
    }

}
}
