#pragma once
#include <efengine/core/Types.h>

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace efengine {
namespace serialization {

    class StringTable {
        public:
            StringTable() { Clear(); }

            // Devuelve el indice de S, insertandola si no estaba. "" siempre es 0.
            u32 Intern(std::string_view s);

            // Fuera de rango devuelve ""
            std::string_view View(u32 index) const;

            u32  Count() const { return static_cast<u32>(m_offsets.size() - 1u); }
            void Clear();

            template <class Ar>
            void Serialize(Ar& ar) {
                ar.Array(m_offsets);
                ar.Array(m_blob);

                if constexpr (Ar::kIsReading) {
                    if (!ar.Ok() || !rebuildAfterRead()) {
                        ar.Fail();
                        Clear();   // no dejar vistas apuntando a un blob inconsistente
                    }
                }
            }

        private:
            // Valida invariantes del chunk
            bool rebuildAfterRead();

            std::vector<u8>  m_blob;
            std::vector<u32> m_offsets;   // size == Count() + 1; back() == m_blob.size()
            std::unordered_map<std::string, u32> m_lookup;
    };

}
}
