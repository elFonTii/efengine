#pragma once

#include <efengine/core/Types.h>
#include <optional>

namespace efengine {
namespace renderer {

    // Posee un Uniform Buffer Object de OpenGL (DSA 4.5, storage inmutable).
    // Handle-owner RO5. El tamaño queda fijo en la creación; el contenido se
    // actualiza con Update (glNamedBufferSubData).
    class UniformBuffer {
        public:
            static std::optional<UniformBuffer> Create(usize sizeBytes);
            ~UniformBuffer();

            UniformBuffer(const UniformBuffer&)            = delete;
            UniformBuffer& operator=(const UniformBuffer&) = delete;
            UniformBuffer(UniformBuffer&& other) noexcept;
            UniformBuffer& operator=(UniformBuffer&& other) noexcept;

            // Engancha el buffer al binding point de GL_UNIFORM_BUFFER indicado
            // (el que declara el shader con layout(std140, binding = N)).
            void BindBase(u32 binding) const;
            void Update(const void* data, usize sizeBytes, usize offset = 0) const;

            u32   id() const { return m_id; }
            usize size() const { return m_size; }

        private:
            UniformBuffer(u32 id, usize size);

            u32   m_id = 0;
            usize m_size = 0;
    };

}
}
