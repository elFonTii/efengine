#include "VertexLayout.h"

#include <efengine/core/Assert.h>

namespace efengine {
namespace renderer {

    u32 ComponentCount(ShaderDataType type) {
        switch (type) {
            case ShaderDataType::Float:  return 1;
            case ShaderDataType::Float2: return 2;
            case ShaderDataType::Float3: return 3;
            case ShaderDataType::Float4: return 4;
        }
        // Un ShaderDataType no contemplado es error de programación (B.7):
        // falla ruidoso en debug; el return es inalcanzable, solo calla al compilador.
        EF_ASSERT(false, "ShaderDataType desconocido en ComponentCount");
        return 0;
    }

    namespace {
        // Tamaño en bytes de un tipo (todos los componentes son f32).
        u32 tamano_bytes(ShaderDataType type) {
            return ComponentCount(type) * static_cast<u32>(sizeof(f32));
        }
    }

    void VertexLayout::Push(ShaderDataType type) {
        const u32 location = static_cast<u32>(m_attributes.size());
        m_attributes.push_back(VertexAttribute{ type, location, m_stride });
        m_stride += tamano_bytes(type);
    }

}
}
