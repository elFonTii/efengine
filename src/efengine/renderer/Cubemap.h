#pragma once
#include <efengine/core/Types.h>
#include <optional>

// https://learnopengl.com/Advanced-OpenGL/Cubemaps
// una textura con mipmaps tiene que guardar la imagen a resolucion completa
// y además una cadena de versiones cada vez más chicas hasta una res 1x1
// nivel 0: 512x512 - nivel 1: 256x256 - nivel 2: 128x128 - etc...


namespace efengine {
namespace renderer {

    inline u32 mipLevels(u32 size) {
        if(size <= 1) return 1;
        
        u32 count = 1;
        while (size > 1) { size >>= 1; ++count; }
        return count;
    }

    class Cubemap {
        public:
            static Cubemap Create(u32 size, u32 internalFormat, u32 mipCount);
            ~Cubemap();

            Cubemap(const Cubemap&)             = delete;
            Cubemap& operator=(const Cubemap&)  = delete;
            Cubemap(Cubemap&& other)          noexcept;
            Cubemap& operator=(Cubemap&& other) noexcept;

            void Bind(u32 unit = 0) const;
            void BindImage(u32 unit, u32 level, u32 access, u32 format) const;
            void GenerateMips() const;

        
            u32 id() const { return m_id; }
            u32 size() const { return m_size; }

        private:
            Cubemap(u32 id, u32 size);

            u32 m_id = 0;
            u32 m_size = 0;
    };
}
}