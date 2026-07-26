#pragma once
#include <efengine/core/Types.h>
#include <efengine/serialization/EfeFile.h>   // kCurrentVersion

#include <cstring>
#include <type_traits>
#include <vector>

namespace efengine {
namespace serialization {

    // Lee de un buffer que NO posee. Toda lectura esta chequeada contra el final:
    // un archivo truncado o corrupto termina con Ok() == false
    class BinaryReader {
        public:
            static constexpr bool kIsReading = true;

            BinaryReader(const u8* data, usize size) : m_data(data), m_size(size) {}
            explicit BinaryReader(const std::vector<u8>& buffer)
                : m_data(buffer.data()), m_size(buffer.size()) {}

            template <class T>
            void Field(T& value) {
                static_assert(std::is_trivially_copyable_v<T>,
                              "BinaryReader::Field solo acepta tipos trivialmente copiables");
                if (!m_ok || Remaining() < sizeof(T)) { value = T{}; Fail(); return; }
                std::memcpy(&value, m_data + m_cursor, sizeof(T));
                m_cursor += sizeof(T);
            }

            void Bytes(void* out, usize count);

            template <class T>
            void Array(std::vector<T>& values) {
                static_assert(std::is_trivially_copyable_v<T>,
                              "BinaryReader::Array solo acepta elementos trivialmente copiables");
                values.clear();

                u32 count = 0u;
                Count(count, sizeof(T));
                if (!m_ok) return;

                if (count > 0u) {
                    values.resize(count);
                    Bytes(values.data(), static_cast<usize>(count) * sizeof(T));
                }
                Align4();
            }

            // Lee una cantidad y verifica que los bytes que promete existan
            void Count(u32& value, usize minElementSize);

            void Align4();

            // Lee el u32 y devuelve el offset donde termina el bloque.
            usize BeginSizedBlock();
            void  EndSizedBlock(usize end);

            bool  Ok() const { return m_ok; }

            // Version del archivo que se esta leyendo. La setea ReadFileHeader;
            // hasta entonces se asume la actual (para buffers armados a mano en tests).
            u32  Version() const { return m_fileVersion; }
            void SetVersion(u32 version) { m_fileVersion = version; }

            usize Remaining() const { return m_ok ? (m_size - m_cursor) : 0u; }
            usize Cursor() const { return m_cursor; }
            usize Size() const { return m_size; }

            void Skip(usize count);
            void Fail() { m_ok = false; }

            void SeekTo(usize offset);

        private:
            const u8* m_data = null;
            usize     m_size = 0u;
            usize     m_cursor = 0u;
            bool      m_ok = true;
            u32       m_fileVersion = kCurrentVersion;
    };

}
}
