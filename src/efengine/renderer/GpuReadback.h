#pragma once
#include <efengine/core/Types.h>

#include <array>

namespace efengine {
namespace renderer {

    class StorageBuffer;

    // Lee un SSBO chico de la GPU sin frenar la CPU. Push copia el buffer a una ranura de
    // staging y pone un fence; Poll devuelve la copia lista mas nueva. El motor no tiene
    // tope de frames en vuelo: si las tres ranuras siguen ocupadas, Push no hace nada.
    class GpuReadback {
        public:
            explicit GpuReadback(usize bytes);
            ~GpuReadback();

            GpuReadback(const GpuReadback&)            = delete;
            GpuReadback& operator=(const GpuReadback&) = delete;
            GpuReadback(GpuReadback&& other) noexcept;
            GpuReadback& operator=(GpuReadback&& other) noexcept;

            // tag viaja con la copia y vuelve en Poll: el frame que la produjo.
            void Push(const StorageBuffer& source, u64 tag);
            bool Poll(void* out, u64& tag);

        private:
            struct Slot {
                u32 buffer = 0u;
                u64 fence  = 0u;
                u64 tag    = 0u;
            };

            void Release();

            std::array<Slot, 3> m_slots {};
            usize               m_bytes = 0u;
    };

}
}
