#pragma once
#include <efecom/RHI.h>
#include <efengine/core/Types.h>
#include <efengine/renderer/VoxelMath.h>

namespace efengine {
namespace renderer {

    // Duenno de las dos texturas 3D del proxy de trazado. No extiende Texture
    // porque Texture es 2D en su interfaz entera (width/height) y meterle una
    // profundidad la ensuciaria para todos sus consumidores.
    class VoxelGrid {
        public:
            static constexpr u32 kAlbedoImageUnit = 0u;
            static constexpr u32 kNormalImageUnit = 1u;

            static VoxelGrid Create(const VoxelGridDesc& desc);

            VoxelGrid() = default;
            ~VoxelGrid();
            VoxelGrid(const VoxelGrid&)            = delete;
            VoxelGrid& operator=(const VoxelGrid&) = delete;
            VoxelGrid(VoxelGrid&& other) noexcept;
            VoxelGrid& operator=(VoxelGrid&& other) noexcept;

            bool valid() const { return m_albedo != 0u && m_normal != 0u; }

            const VoxelGridDesc& desc() const { return m_desc; }
            u32 albedoId() const { return m_albedo; }
            u32 normalId() const { return m_normal; }

            // Bytes de VRAM de las dos texturas. Sale al panel.
            u64 memoryBytes() const;

            // Deja las dos en cero. El storage inmutable arranca con contenido
            // INDEFINIDO, y un alfa indefinido hace que el DDA pegue contra
            // basura en el primer frame. Mismo razonamiento que
            // DdgiPass::ClearAtlas.
            void Clear();

            void BindForWrite() const;    // imagenes, para voxelize.frag
            void BindForSample(u32 albedoUnit, u32 normalUnit) const;  // samplers, para el trazado

        private:
            VoxelGrid(const VoxelGridDesc& desc, u32 albedo, u32 normal);

            VoxelGridDesc m_desc   {};
            u32           m_albedo = 0u;
            u32           m_normal = 0u;
    };

}
}
