#pragma once

#include <efengine/core/Types.h>
#include <optional>

namespace efengine {
namespace renderer {

    // Posee un programa de shaders de OpenGL. Handle-owner RO5.
    // Invariante fuerte: todo Shader que existe compiló y linkeó con éxito.
    // La compilación puede fallar (recuperable) → factory Create devuelve
    // nullopt en vez de abortar (CLAUDE.md B.8).
    //
    // No tiene setters de uniform: todo dato de shader viaja por UBO en un
    // binding explicito (renderer/ShaderBlocks.h). Vulkan no tiene default
    // uniform block, asi que esta clase no puede volver a tenerlo.
    class Shader {
        public:
            static std::optional<Shader> Create(const char* vertexSrc, const char* fragmentSrc);
            static std::optional<Shader> CreateCompute(const char* computeSrc);

            ~Shader();

            Shader(const Shader&)            = delete;
            Shader& operator=(const Shader&) = delete;
            Shader(Shader&& other) noexcept;
            Shader& operator=(Shader&& other) noexcept;

            void Bind() const;
            u32  id() const { return m_program; }

        private:
            explicit Shader(u32 program);

            u32 m_program = 0;
    };

}
}
