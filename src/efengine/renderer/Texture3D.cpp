#include "Texture3D.h"

#include <glad/gl.h>
#include <utility>

#include <efengine/core/Assert.h>

namespace efengine {
namespace renderer {

    std::optional<Texture3D> Texture3D::Create(u32 width, u32 height, u32 depth,
                                               u32 internalFormat, u32 mipCount) {
        EF_ASSERT(width > 0 && height > 0 && depth > 0, "Texture3D::Create: dimensiones invalidas");
        EF_ASSERT(mipCount >= 1, "Texture3D::Create: mipCount debe ser >= 1");

        u32 id = 0;
        glCreateTextures(GL_TEXTURE_3D, 1, &id);
        EF_ASSERT(id != 0, "Texture3D::Create: glCreateTextures devuelve 0 (sin contexto GL)");

        glTextureStorage3D(id, (GLsizei)mipCount, internalFormat,
                           (GLsizei)width, (GLsizei)height, (GLsizei)depth);

        // Muestreo trilinear con clamp: los volúmenes de GI no se repiten.
        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, mipCount > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        return Texture3D(id, width, height, depth);
    }

    Texture3D::Texture3D(u32 id, u32 width, u32 height, u32 depth)
        : m_id(id), m_width(width), m_height(height), m_depth(depth) {}

    Texture3D::~Texture3D() {
        if (m_id != 0) glDeleteTextures(1, &m_id);
    }

    Texture3D::Texture3D(Texture3D&& other) noexcept
        : m_id(std::exchange(other.m_id, 0))
        , m_width(std::exchange(other.m_width, 0))
        , m_height(std::exchange(other.m_height, 0))
        , m_depth(std::exchange(other.m_depth, 0)) {}

    Texture3D& Texture3D::operator=(Texture3D&& other) noexcept {
        if (this != &other) {
            if (m_id != 0) glDeleteTextures(1, &m_id);
            m_id     = std::exchange(other.m_id, 0);
            m_width  = std::exchange(other.m_width, 0);
            m_height = std::exchange(other.m_height, 0);
            m_depth  = std::exchange(other.m_depth, 0);
        }
        return *this;
    }

    void Texture3D::Bind(u32 unit) const {
        EF_ASSERT(m_id != 0, "Texture3D::Bind: textura vacia (movida o no inicializada)");
        glBindTextureUnit(unit, m_id);
    }

    void Texture3D::BindImage(u32 unit, u32 level, u32 access, u32 format) const {
        EF_ASSERT(m_id != 0, "Texture3D::BindImage: textura vacia (movida o no inicializada)");
        glBindImageTexture(unit, m_id, (GLint)level, GL_TRUE, 0, access, format);
    }

    void Texture3D::Clear(u32 level) const {
        EF_ASSERT(m_id != 0, "Texture3D::Clear: textura vacia (movida o no inicializada)");
        glClearTexImage(m_id, (GLint)level, GL_RGBA, GL_UNSIGNED_BYTE, null);
    }

}
}
