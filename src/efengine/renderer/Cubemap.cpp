#include "efengine/renderer/Cubemap.h"
#include "efengine/core/Assert.h"
#include <utility>

#include "glad/gl.h"

namespace efengine {
namespace renderer {

    Cubemap Cubemap::Create(u32 size, u32 internalFormat, u32 mipCount) {
        u32 id = 0;
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);
        EF_ASSERT(id != 0, "Cubemap::Create: glCreateTextures devuelve 0 (sin contexto GL)");
        glTextureStorage2D(id, mipCount, internalFormat, size, size); // storage2d reserva 6 caras para el cubemap

        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        return Cubemap(id, size);
    }

    void Cubemap::Bind(u32 unit) const { glBindTextureUnit(unit, m_id); }
    void Cubemap::BindImage(u32 unit, u32 level, u32 access, u32 format) const { glBindImageTexture(unit, m_id, level, GL_TRUE, 0, access, format); }
    void Cubemap::GenerateMips() const { glGenerateTextureMipmap(m_id); }

    Cubemap::Cubemap(u32 id, u32 size) : m_id(id), m_size(size) {}

    Cubemap::~Cubemap() { if(m_id != 0) { glDeleteTextures(1, &m_id); }}

    Cubemap::Cubemap(Cubemap&& other) noexcept
        : m_id(std::exchange(other.m_id, 0))
        , m_size(std::exchange(other.m_size, 0)) {}
    
    Cubemap& Cubemap::operator=(Cubemap&& other) noexcept {
        if(this != &other) {
            if(m_id != 0) { glDeleteTextures(1, &m_id); }

            m_id = std::exchange(other.m_id, 0);
            m_size = std::exchange(other.m_size, 0);
        }
        return *this;
    }

}
}