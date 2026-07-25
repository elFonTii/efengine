#pragma once

#include <efengine/core/Types.h>
#include <optional>

namespace efengine {
namespace renderer {

    // Posee un Shader Storage Buffer Object de OpenGL (DSA 4.5, storage
    // inmutable). Handle-owner RO5. Pensado para arrays grandes leídos y/o
    // escritos por compute shaders (std430): resultados de rayos de probes, etc.
    class ShaderStorageBuffer {
        public:
            // 'flags' son los de glNamedBufferStorage; por defecto el contenido
            // solo lo escribe la GPU. Pasar GL_DYNAMIC_STORAGE_BIT si la CPU
            // necesita Update.
            static std::optional<ShaderStorageBuffer> Create(usize sizeBytes, u32 flags = 0);
            ~ShaderStorageBuffer();

            ShaderStorageBuffer(const ShaderStorageBuffer&)            = delete;
            ShaderStorageBuffer& operator=(const ShaderStorageBuffer&) = delete;
            ShaderStorageBuffer(ShaderStorageBuffer&& other) noexcept;
            ShaderStorageBuffer& operator=(ShaderStorageBuffer&& other) noexcept;

            // Engancha al binding point de GL_SHADER_STORAGE_BUFFER indicado
            // (el que declara el shader con layout(std430, binding = N)).
            void BindBase(u32 binding) const;
            // Requiere GL_DYNAMIC_STORAGE_BIT en la creación.
            void Update(const void* data, usize sizeBytes, usize offset = 0) const;

            u32   id() const { return m_id; }
            usize size() const { return m_size; }

        private:
            ShaderStorageBuffer(u32 id, usize size);

            u32   m_id = 0;
            usize m_size = 0;
    };

}
}
