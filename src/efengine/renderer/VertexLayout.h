#pragma once

#include <efengine/core/Types.h>
#include <vector>

namespace efengine {
namespace renderer {

    // Tipos de dato de un atributo de vértice (todos f32 por ahora).
    enum class ShaderDataType { Float, Float2, Float3, Float4 };

    struct VertexAttribute {
        ShaderDataType type;
        u32            location;   // asignada por orden de inserción
        u32            offset;     // bytes desde el inicio del vértice
    };

    // Nº de componentes (floats) de un tipo de atributo.
    u32 ComponentCount(ShaderDataType type);

    // Describe el layout de atributos de un vértice. No posee recursos GPU
    // (Rule of Zero): solo acumula la descripción que VertexArray consume.
    class VertexLayout {
        public:
            void Push(ShaderDataType type);   // location = orden de inserción
            u32  stride() const { return m_stride; }
            const std::vector<VertexAttribute>& attributes() const { return m_attributes; }

        private:
            std::vector<VertexAttribute> m_attributes;
            u32                          m_stride = 0;
    };

}
}
