#pragma once

#include <efengine/core/Types.h>
#include <optional>

namespace efengine {
namespace renderer {

    // Posee una textura 3D inmutable de OpenGL (DSA 4.5, glTextureStorage3D).
    // Handle-owner RO5. Pensada para volúmenes de GI: clipmaps de vóxeles,
    // SDFs y cachés de radiancia escritos por compute (BindImage).
    class Texture3D {
        public:
            static std::optional<Texture3D> Create(u32 width, u32 height, u32 depth,
                                                   u32 internalFormat, u32 mipCount = 1);
            ~Texture3D();

            Texture3D(const Texture3D&)            = delete;
            Texture3D& operator=(const Texture3D&) = delete;
            Texture3D(Texture3D&& other) noexcept;
            Texture3D& operator=(Texture3D&& other) noexcept;

            void Bind(u32 unit) const;
            // Bindea el nivel entero (layered) como imagen de lectura/escritura
            // para compute. 'access'/'format' son los enums crudos de GL, igual
            // que en Cubemap::BindImage.
            void BindImage(u32 unit, u32 level, u32 access, u32 format) const;
            // Borra el nivel a cero (glClearTexImage): reset de una cascada
            // antes de re-voxelizar.
            void Clear(u32 level) const;

            u32 id() const { return m_id; }
            u32 width() const { return m_width; }
            u32 height() const { return m_height; }
            u32 depth() const { return m_depth; }

        private:
            Texture3D(u32 id, u32 width, u32 height, u32 depth);

            u32 m_id = 0;
            u32 m_width = 0;
            u32 m_height = 0;
            u32 m_depth = 0;
    };

}
}
