#include "Texture.h"

#include <glad/gl.h>
#include <utility>
#include <stb_image.h>

#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>

namespace efengine {
namespace renderer {
    std::optional<Texture> Texture::Create(const char* path, ColorSpace color_space) {
        EF_ASSERT(path != null, "Texture:Create: Path no puede ser null");

        stbi_set_flip_vertically_on_load(true);

        i32 width = 0;
        i32 height = 0;
        i32 channels = 0; // stbi_load lo setea por referencia
        u8* pixels = stbi_load(path, &width, &height, &channels, 0);
        GLenum format = 0; // en que formato está la textura
        GLenum internalFormat = 0; // en que formato la gpu interpreta la textura

        if(pixels == null) {
            EF_LOG_ERROR("Texture::Create: fallo al cargar '%s': %s", path, stbi_failure_reason());
            return std::nullopt; // Fallo recuperable
        }

        switch(color_space) {
            case ColorSpace::sRGB:
                internalFormat = (channels == 4) ? GL_SRGB8_ALPHA8 : GL_SRGB8;
                break;
            case ColorSpace::Linear:
                internalFormat = (channels == 4) ? GL_RGBA8 : GL_RGB8;
                break;
            default:
                break;
        }

        switch(channels) {
            case 4:
                format = GL_RGBA;
                break;
            case 3:
                format = GL_RGB;
                break;
            case 2:
                format = GL_RG;
                break;
            case 1:
                format = GL_RED;
                break;
            default:
                EF_LOG_ERROR("Texture::Create: fallo al determinar cantidad de canales.");
                stbi_image_free(pixels);
                return std::nullopt;
        }


        u32 id = 0;
        glGenTextures(1, &id);
        EF_ASSERT(id != 0, "Texture::Create: No hay contexto GL");

        glBindTexture(GL_TEXTURE_2D, id);

        // Parámetros de textura
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // Subir los pixeles a la GPU
        glTexImage2D(GL_TEXTURE_2D, 0, (GLint)internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Liberamos el buffer de CPU (ya está en GPU)
        stbi_image_free(pixels); 
        glBindTexture(GL_TEXTURE_2D, 0);

        return Texture(id, (u32)width, (u32)height);
    }

    Texture::Texture(u32 id, u32 width, u32 height) {
        m_id = id;
        m_width = width;
        m_height = height;
    }

    Texture::~Texture() {
        if(m_id != 0) {
            glDeleteTextures(1, &m_id);
        }
    } 

    Texture::Texture(Texture&& other) noexcept
        : m_id(std::exchange(other.m_id, 0))
        , m_width(std::exchange(other.m_width, 0))
        , m_height(std::exchange(other.m_height, 0)) {}

    Texture& Texture::operator=(Texture&& other) noexcept {
        if(this != &other) {
            if(m_id != 0) {
                glDeleteTextures(1, &m_id);
            }
            m_id = std::exchange(other.m_id, 0);
            m_width = std::exchange(other.m_width, 0);
            m_height = std::exchange(other.m_height, 0);
        }
        return *this;
    }

    void Texture::Bind(u32 unit) const {
        EF_ASSERT(m_id != 0, "Texture::Bind: Textura no válida");
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, m_id);
    }
}
}