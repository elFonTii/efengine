#pragma once

#include <efengine/core/Types.h>
#include <optional>

namespace efengine {
namespace renderer {

    // Posee un programa de shaders de OpenGL. Handle-owner RO5.
    // Invariante fuerte: todo Shader que existe compiló y linkeó con éxito.
    // La compilación puede fallar (recuperable) → factory Create devuelve
    // nullopt en vez de abortar (CLAUDE.md B.8).
    class Shader {
        public:
            static std::optional<Shader> Create(const char* vertexSrc, const char* fragmentSrc);
            ~Shader();

            Shader(const Shader&)            = delete;
            Shader& operator=(const Shader&) = delete;
            Shader(Shader&& other) noexcept;
            Shader& operator=(Shader&& other) noexcept;

            void Bind() const;
            u32  id() const { return m_program; }

        private:
            explicit Shader(u32 program);   // privado: solo Create, ya validado
            u32 m_program = 0;
    };

}
}
