#include "efengine/serialization/BinaryWriter.h"
#include <efengine/core/Assert.h>
#include <utility>

namespace efengine {
namespace serialization {

    void BinaryWriter::Bytes(const void* data, usize count) {
        if (count == 0u) return;
        EF_ASSERT(data != null, "BinaryWriter::Bytes: data nula con count > 0");

        const usize offset = m_buffer.size();
        m_buffer.resize(offset + count);
        std::memcpy(m_buffer.data() + offset, data, count);
    }

    void BinaryWriter::Count(u32& value, usize /*minElementSize*/) {
        Field(value);
    }

    void BinaryWriter::Align4() {
        while ((m_buffer.size() % 4u) != 0u) m_buffer.push_back(0u);
    }

    usize BinaryWriter::BeginSizedBlock() {
        const usize marker = m_buffer.size();
        u32 placeholder = 0u;
        Field(placeholder);
        return marker;
    }

    void BinaryWriter::EndSizedBlock(usize marker) {
        Align4();
        const usize contentStart = marker + sizeof(u32);
        EF_ASSERT(m_buffer.size() >= contentStart,
                  "BinaryWriter::EndSizedBlock: marker invalido");
        PatchU32(marker, static_cast<u32>(m_buffer.size() - contentStart));
    }

    std::vector<u8> BinaryWriter::Take() {
        std::vector<u8> out = std::move(m_buffer);
        m_buffer.clear();
        return out;
    }

    void BinaryWriter::PatchU32(usize offset, u32 value) {
        EF_ASSERT(offset + sizeof(u32) <= m_buffer.size(),
                  "BinaryWriter::PatchU32: offset fuera del buffer");
        std::memcpy(m_buffer.data() + offset, &value, sizeof(u32));
    }

}
}
