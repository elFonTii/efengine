#include "efengine/renderer/Cubemap.h"
#include "efengine/core/Assert.h"
#include <utility>

#include <efecom/RHI.h>

namespace efengine {
namespace renderer {

    Cubemap Cubemap::Create(u32 size, efecom::TextureFormat format, u32 mipCount) {
        const u32 id = efecom::CreateCubemap(size, format, mipCount);
        EF_ASSERT(id != 0, "Cubemap::Create: efecom::CreateCubemap devuelve 0 (sin contexto GL)");

        return Cubemap(id, size);
    }

    void Cubemap::Bind(u32 unit) const { efecom::BindTextureUnit(m_id, unit); }
    void Cubemap::BindImage(u32 unit, u32 level, efecom::ImageAccess access, efecom::TextureFormat format) const { efecom::BindImageLayered(unit, m_id, level, access, format); }
    void Cubemap::GenerateMips() const { efecom::GenerateTextureMipmaps(m_id); }

    Cubemap::Cubemap(u32 id, u32 size) : m_id(id), m_size(size) {}

    Cubemap::~Cubemap() { if(m_id != 0) { efecom::DestroyTexture(m_id); }}

    Cubemap::Cubemap(Cubemap&& other) noexcept
        : m_id(std::exchange(other.m_id, 0))
        , m_size(std::exchange(other.m_size, 0)) {}

    Cubemap& Cubemap::operator=(Cubemap&& other) noexcept {
        if(this != &other) {
            if(m_id != 0) { efecom::DestroyTexture(m_id); }

            m_id = std::exchange(other.m_id, 0);
            m_size = std::exchange(other.m_size, 0);
        }
        return *this;
    }

}
}
