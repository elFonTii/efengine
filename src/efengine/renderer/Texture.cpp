#include "Texture.h"

#include <efecom/RHI.h>
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

        if(pixels == null) {
            EF_LOG_ERROR("Texture::Create: fallo al cargar '%s': %s", path, stbi_failure_reason());
            return std::nullopt; // Fallo recuperable
        }

        // El formato del RHI determina a la vez cómo la guarda la GPU y el
        // layout de canales del origen (1-2 canales siempre lineales: sRGB
        // solo existe para RGB/RGBA).
        efecom::TextureFormat format;
        switch(channels) {
            case 4:
                format = (color_space == ColorSpace::sRGB) ? efecom::TextureFormat::SRGB8_A8
                                                           : efecom::TextureFormat::RGBA8;
                break;
            case 3:
                format = (color_space == ColorSpace::sRGB) ? efecom::TextureFormat::SRGB8
                                                           : efecom::TextureFormat::RGB8;
                break;
            case 2:
                format = efecom::TextureFormat::RG8;
                break;
            case 1:
                format = efecom::TextureFormat::R8;
                break;
            default:
                EF_LOG_ERROR("Texture::Create: fallo al determinar cantidad de canales.");
                stbi_image_free(pixels);
                return std::nullopt;
        }

        efecom::Texture2DDesc desc;
        desc.width  = (u32)width;
        desc.height = (u32)height;
        desc.format = format;
        desc.minFilter = efecom::TextureFilter::LinearMipmapLinear;
        desc.magFilter = efecom::TextureFilter::Linear;
        desc.wrapS = efecom::TextureWrap::Repeat;
        desc.wrapT = efecom::TextureWrap::Repeat;
        desc.generateMipmaps = true;

        const u32 id = efecom::CreateTexture2D(desc, pixels);
        EF_ASSERT(id != 0, "Texture::Create: No hay contexto GL");

        // Liberamos el buffer de CPU (ya está en GPU)
        stbi_image_free(pixels);

        return Texture(id, (u32)width, (u32)height);
    }
    std::optional<Texture> Texture::CreateHDR(const char* path) {
        EF_ASSERT(path != null, "Texture::CreateHDR: Path no puede ser null");

        // algo de que la proyección define la orientación
        stbi_set_flip_vertically_on_load(false);

        i32 width = 0, height = 0, channels = 0;
        float* data = stbi_loadf(path, &width, &height, &channels, 4);

        stbi_set_flip_vertically_on_load(true);

        if (data == null) {
            EF_LOG_ERROR("Texture::CreateHDR: fallo al cargar '%s': %s", path, stbi_failure_reason());
            return std::nullopt;
        }

        efecom::Texture2DDesc desc;
        desc.width  = (u32)width;
        desc.height = (u32)height;
        desc.format = efecom::TextureFormat::RGBA16F;
        desc.minFilter = efecom::TextureFilter::Linear;
        desc.magFilter = efecom::TextureFilter::Linear;
        desc.wrapS = efecom::TextureWrap::Repeat;
        desc.wrapT = efecom::TextureWrap::ClampToEdge;

        const u32 id = efecom::CreateTexture2D(desc, data);
        EF_ASSERT(id != 0, "Texture::CreateHDR: No hay contexto GL");

        stbi_image_free(data);

        return Texture(id, (u32)width, (u32)height);
    }

    // Textura vacía para usar como color attachment de un framebuffer
    Texture Texture::CreateColorAttachment(u32 width, u32 height) {
        // Attachment HDR (pre-tonemapping)
        efecom::Texture2DDesc desc;
        desc.width  = width;
        desc.height = height;
        desc.format = efecom::TextureFormat::RGBA16F;
        desc.minFilter = efecom::TextureFilter::Linear;
        desc.magFilter = efecom::TextureFilter::Linear;
        desc.wrapS = efecom::TextureWrap::ClampToEdge;
        desc.wrapT = efecom::TextureWrap::ClampToEdge;

        const u32 id = efecom::CreateTexture2D(desc, nullptr);
        EF_ASSERT(id != 0, "Texture::CreateColorAttachment: No hay contexto GL");

        return Texture(id, width, height);
    }

    Texture Texture::CreateDepthAttachment(u32 width, u32 height) {
        efecom::Texture2DDesc desc;
        desc.width  = width;
        desc.height = height;
        desc.format = efecom::TextureFormat::Depth32F;
        // PCF manual → NEAREST (el filtrado 3×3 lo hace el fragment, no el sampler).
        desc.minFilter = efecom::TextureFilter::Nearest;
        desc.magFilter = efecom::TextureFilter::Nearest;
        // Fuera del frustum → profundidad de borde 1.0 → iluminado (no en sombra).
        desc.wrapS = efecom::TextureWrap::ClampToBorder;
        desc.wrapT = efecom::TextureWrap::ClampToBorder;
        desc.borderColor[0] = 1.0f;
        desc.borderColor[1] = 1.0f;
        desc.borderColor[2] = 1.0f;
        desc.borderColor[3] = 1.0f;

        const u32 id = efecom::CreateTexture2D(desc, nullptr);
        EF_ASSERT(id != 0, "Texture::CreateDepthAttachment: No hay contexto GL");

        return Texture(id, width, height);
    }

    Texture::Texture(u32 id, u32 width, u32 height) {
        m_id = id;
        m_width = width;
        m_height = height;
    }

    Texture::~Texture() {
        if(m_id != 0) {
            efecom::DestroyTexture(m_id);
        }
    }

    Texture::Texture(Texture&& other) noexcept
        : m_id(std::exchange(other.m_id, 0))
        , m_width(std::exchange(other.m_width, 0))
        , m_height(std::exchange(other.m_height, 0)) {}

    Texture& Texture::operator=(Texture&& other) noexcept {
        if(this != &other) {
            if(m_id != 0) {
                efecom::DestroyTexture(m_id);
            }
            m_id = std::exchange(other.m_id, 0);
            m_width = std::exchange(other.m_width, 0);
            m_height = std::exchange(other.m_height, 0);
        }
        return *this;
    }

    void Texture::Bind(u32 unit) const {
        EF_ASSERT(m_id != 0, "Texture::Bind: Textura no válida");
        efecom::BindTexture2D(m_id, unit);
    }
}
}
