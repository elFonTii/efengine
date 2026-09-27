#pragma once

#include <efengine/core/Types.h>

namespace efengine {
namespace renderer {

    // Posee un SSBO. Mismo contrato que UniformBuffer: tamano fijo, se reescribe con Update().
    class StorageBuffer {
        public:
            explicit StorageBuffer(usize size);
            ~StorageBuffer();

            StorageBuffer(const StorageBuffer&)            = delete;
            StorageBuffer& operator=(const StorageBuffer&) = delete;
            StorageBuffer(StorageBuffer&& other) noexcept;
            StorageBuffer& operator=(StorageBuffer&& other) noexcept;

            void Update(const void* data, usize size, usize offset = 0u) const;

            // Binding de SSBO, no de UBO: son espacios de indices distintos.
            void BindTo(u32 bindingIndex) const;

            u32   id()   const { return m_id; }
            usize size() const { return m_size; }

        private:
            u32   m_id   = 0;
            usize m_size = 0;
    };

}
}
