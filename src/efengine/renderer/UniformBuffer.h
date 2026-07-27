#pragma once

#include <efengine/core/Types.h>

namespace efengine {
namespace renderer {

    // Posee un buffer de uniforms. Handle-owner RO5, igual que Buffer: adquiere
    // en el ctor, libera en el dtor, no-copiable, movible.
    //
    // El tamano es fijo: se reserva una vez y se reescribe el contenido con
    // Update(). Ese es el patron mas simple que funciona para el volumen de draws
    // de hoy; si algun dia hace falta, el upgrade es un ring buffer con offsets
    // dinamicos y NO toca los shaders.
    class UniformBuffer {
        public:
            explicit UniformBuffer(usize size);
            ~UniformBuffer();

            UniformBuffer(const UniformBuffer&)            = delete;
            UniformBuffer& operator=(const UniformBuffer&) = delete;
            UniformBuffer(UniformBuffer&& other) noexcept;
            UniformBuffer& operator=(UniformBuffer&& other) noexcept;

            // Sube 'size' bytes desde 'data' al offset dado. El caller responde
            // por que offset + size no pase del tamano reservado.
            void Update(const void* data, usize size, usize offset = 0u) const;

            // Engancha este buffer a un indice de binding. Estado global: hacerlo
            // una vez alcanza, salvo que otro buffer se enganche al mismo indice.
            void BindTo(u32 bindingIndex) const;

            u32   id()   const { return m_id; }
            usize size() const { return m_size; }

        private:
            u32   m_id   = 0;
            usize m_size = 0;
    };

}
}
