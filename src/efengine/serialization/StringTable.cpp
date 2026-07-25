#include "efengine/serialization/StringTable.h"

namespace efengine {
namespace serialization {

    void StringTable::Clear() {
        m_blob.clear();
        m_offsets.assign(2u, 0u);   // un solo string ("") de largo 0
        m_lookup.clear();
        m_lookup.emplace(std::string{}, 0u);
    }

    u32 StringTable::Intern(std::string_view s) {
        const std::string key(s);
        if (auto it = m_lookup.find(key); it != m_lookup.end()) return it->second;

        const u32 index = Count();
        m_blob.insert(m_blob.end(), s.begin(), s.end());
        m_offsets.push_back(static_cast<u32>(m_blob.size()));
        m_lookup.emplace(key, index);
        return index;
    }

    std::string_view StringTable::View(u32 index) const {
        if (index >= Count()) return std::string_view{};

        const u32 begin = m_offsets[index];
        const u32 end   = m_offsets[index + 1u];
        return std::string_view(reinterpret_cast<const char*>(m_blob.data()) + begin,
                                static_cast<usize>(end - begin));
    }

    bool StringTable::rebuildAfterRead() {
        // se valida todo antes.
        if (m_offsets.size() < 2u)               return false;
        if (m_offsets.front() != 0u)             return false;
        if (m_offsets.back() != m_blob.size())   return false;
        for (usize i = 1u; i < m_offsets.size(); ++i) {
            if (m_offsets[i] < m_offsets[i - 1u]) return false;
        }

        m_lookup.clear();
        const u32 count = Count();
        for (u32 i = 0u; i < count; ++i) {
            m_lookup.emplace(std::string(View(i)), i);
        }
        return true;
    }

}
}
