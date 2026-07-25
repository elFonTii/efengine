#include "efengine/serialization/BinaryReader.h"

namespace efengine {
namespace serialization {

    void BinaryReader::Bytes(void* out, usize count) {
        if (count == 0u) return;
        if (!m_ok || Remaining() < count) { Fail(); return; }
        std::memcpy(out, m_data + m_cursor, count);
        m_cursor += count;
    }

    void BinaryReader::Count(u32& value, usize minElementSize) {
        Field(value);
        if (!m_ok) { value = 0u; return; }

        if (minElementSize > 0u) {
            const usize maxElems = Remaining() / minElementSize;
            if (static_cast<usize>(value) > maxElems) { value = 0u; Fail(); }
        }
    }

    void BinaryReader::Align4() {
        if (!m_ok) return;
        const usize resto = m_cursor % 4u;
        if (resto == 0u) return;
        Skip(4u - resto);
    }

    usize BinaryReader::BeginSizedBlock() {
        u32 size = 0u;
        Field(size);
        if (!m_ok) return 0u;

        if (Remaining() < static_cast<usize>(size)) { Fail(); return 0u; }
        return m_cursor + static_cast<usize>(size);
    }

    void BinaryReader::EndSizedBlock(usize end) {
        SeekTo(end);   // descarta lo que el lector no consumio
    }

    void BinaryReader::Skip(usize count) {
        if (!m_ok || Remaining() < count) { Fail(); return; }
        m_cursor += count;
    }

    void BinaryReader::SeekTo(usize offset) {
        if (!m_ok || offset > m_size) { Fail(); return; }
        m_cursor = offset;
    }

}
}
