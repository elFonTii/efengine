#include "efengine/renderer/GpuReadback.h"

#include <efecom/RHI.h>

#include <efengine/core/Assert.h>
#include <efengine/renderer/StorageBuffer.h>

#include <utility>

namespace efengine {
namespace renderer {

    GpuReadback::GpuReadback(usize bytes) : m_bytes(bytes) {
        EF_ASSERT(bytes > 0u, "GpuReadback: tamano cero");
        for (Slot& s : m_slots) s.buffer = efecom::CreateStorageBuffer(bytes);
    }

    GpuReadback::~GpuReadback() { Release(); }

    GpuReadback::GpuReadback(GpuReadback&& other) noexcept
        : m_slots(std::exchange(other.m_slots, {}))
        , m_bytes(std::exchange(other.m_bytes, 0u)) {}

    GpuReadback& GpuReadback::operator=(GpuReadback&& other) noexcept {
        if (this != &other) {
            Release();
            m_slots = std::exchange(other.m_slots, {});
            m_bytes = std::exchange(other.m_bytes, 0u);
        }
        return *this;
    }

    void GpuReadback::Release() {
        for (Slot& s : m_slots) {
            if (s.fence  != 0u) efecom::DestroyFence(s.fence);
            if (s.buffer != 0u) efecom::DestroyBuffer(s.buffer);
            s = Slot{};
        }
    }

    void GpuReadback::Push(const StorageBuffer& source, u64 tag) {
        EF_ASSERT(source.size() >= m_bytes, "GpuReadback::Push: el origen es mas chico que la ranura");
        for (Slot& s : m_slots) {
            if (s.buffer == 0u || s.fence != 0u) continue;
            efecom::CopyBuffer(source.id(), s.buffer, 0u, 0u, m_bytes);
            s.fence = efecom::CreateFence();
            s.tag   = tag;
            return;
        }
    }

    bool GpuReadback::Poll(void* out, u64& tag) {
        Slot* mejor = null;
        for (Slot& s : m_slots) {
            if (s.fence == 0u || !efecom::FenceSignaled(s.fence)) continue;
            if (mejor == null || s.tag > mejor->tag) mejor = &s;
        }
        if (mejor == null) return false;

        efecom::ReadBuffer(mejor->buffer, 0u, m_bytes, out);
        tag = mejor->tag;

        // Las listas mas viejas ya no sirven: se liberan para el proximo Push.
        for (Slot& s : m_slots) {
            if (s.fence != 0u && s.tag <= tag) {
                efecom::DestroyFence(s.fence);
                s.fence = 0u;
            }
        }
        return true;
    }

}
}
