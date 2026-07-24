#pragma once

#include <efengine/core/Types.h>
#include <optional>

namespace efengine {
namespace renderer {
    enum class ColorSpace { Linear, sRGB};

    class Texture {
        public:
            static std::optional<Texture> Create(const char* path, ColorSpace color_space = ColorSpace::Linear);
            static std::optional<Texture> CreateHDR(const char* path);
            static Texture CreateColorAttachment(u32 width, u32 height);
            ~Texture();

            Texture(const Texture&)             = delete; // deshabilita copia
            Texture& operator=(const Texture&)  = delete;
            Texture(Texture&& other)          noexcept; // constructor de movimiento
            Texture& operator=(Texture&& other) noexcept; // permite que una textura adquiera la propiedad de la copia

            void Bind(u32 unit = 0) const;
        
            u32 id() const { return m_id; }
            u32 width() const { return m_width; }
            u32 height() const { return m_height; }

        private:
            Texture(u32 id, u32 width, u32 height);
            u32 m_id = 0;
            u32 m_width = 0;
            u32 m_height = 0;
    };
}
}