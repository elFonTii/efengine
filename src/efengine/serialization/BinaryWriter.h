#pragma once
#include <efengine/core/Types.h>

#include <cstring>
#include <type_traits>
#include <vector>

namespace efengine {
namespace serialization {

    // Escribe a un buffer en memoria, clase identica a
    // BinaryReader, lo que permite que una sola funcion Serialize sirva para ambas
    class BinaryWriter {
        public:
            static constexpr bool kIsReading = false;

            template <class T>
            void Field(T& value) {
                static_assert(std::is_trivially_copyable_v<T>,
                              "BinaryWriter::Field solo acepta tipos trivialmente copiables");
                Bytes(&value, sizeof(T));
            }

            void Bytes(const void* data, usize count);

            template <class T>
            void Array(std::vector<T>& values) {
                static_assert(std::is_trivially_copyable_v<T>,
                              "BinaryWriter::Array solo acepta elementos trivialmente copiables");
                u32 count = static_cast<u32>(values.size());
                Field(count);
                if (count > 0u) Bytes(values.data(), count * sizeof(T));
                Align4();
            }

            void Count(u32& value, usize minElementSize);

            void Align4();

            usize BeginSizedBlock();
            void  EndSizedBlock(usize marker);

            bool  Ok() const { return true; }
            usize Size() const { return m_buffer.size(); }

            const std::vector<u8>& Buffer() const { return m_buffer; }
            std::vector<u8>        Take();

            void PatchU32(usize offset, u32 value);

        private:
            std::vector<u8> m_buffer;
    };

}
}
