#include "Shader.h"

#include <efecom/RHI.h>
#include <utility>
#include <glm/gtc/type_ptr.hpp>

#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>

namespace efengine {
namespace renderer {

    namespace {
        u32 compilar_stage(efecom::ShaderStage tipo, const char* src) {
            char log[512] = {};
            const u32 stage = efecom::CompileShader(tipo, src, log, sizeof(log));
            if (stage == 0) {
                EF_LOG_ERROR("Shader: fallo al compilar stage: %s", log);
                return 0;
            }
            return stage;
        }
    }

    std::optional<Shader> Shader::Create(const char* vertexSrc, const char* fragmentSrc) {
        EF_ASSERT(vertexSrc != null, "Shader::Create: vertexSrc no puede ser null");
        EF_ASSERT(fragmentSrc != null, "Shader::Create: fragmentSrc no puede ser null");

        const u32 vs = compilar_stage(efecom::ShaderStage::Vertex, vertexSrc);
        if (vs == 0) {
            return std::nullopt;
        }

        const u32 fs = compilar_stage(efecom::ShaderStage::Fragment, fragmentSrc);
        if (fs == 0) {
            efecom::DestroyShader(vs);
            return std::nullopt;
        }

        char log[512] = {};
        const u32 program = efecom::LinkProgram(vs, fs, log, sizeof(log));

        // Los stages ya no se necesitan tras linkear.
        efecom::DestroyShader(vs);
        efecom::DestroyShader(fs);

        if (program == 0) {
            EF_LOG_ERROR("Shader: fallo al linkear el programa: %s", log);
            return std::nullopt;
        }

        return Shader(program);
    }

     std::optional<Shader> Shader::CreateCompute(const char* computeSrc) {
        EF_ASSERT(computeSrc != null, "Shader::CreateCompute: computeSrc no puede ser null");

        const u32 comp = compilar_stage(efecom::ShaderStage::Compute, computeSrc);
        if (comp == 0) {
            return std::nullopt;
        }

        char log[512] = {};
        const u32 program = efecom::LinkProgram(comp, 0, log, sizeof(log));

        // El stage ya no se necesita tras linkear.
        efecom::DestroyShader(comp);

        if (program == 0) {
            EF_LOG_ERROR("Shader::CreateCompute: fallo al linkear el programa: %s", log);
            return std::nullopt;
        }

        return Shader(program);
    }

    Shader::Shader(u32 program) : m_program(program) {}

    Shader::~Shader() {
        if (m_program != 0) {
            efecom::DestroyProgram(m_program);
        }
    }

    Shader::Shader(Shader&& other) noexcept
        : m_program(std::exchange(other.m_program, 0)) {}

    Shader& Shader::operator=(Shader&& other) noexcept {
        if (this != &other) {
            if (m_program != 0) {
                efecom::DestroyProgram(m_program);
            }
            m_program = std::exchange(other.m_program, 0);
        }
        return *this;
    }

    void Shader::Bind() const {
        EF_ASSERT(m_program != 0, "Shader::Bind: programa vacio (movido o no inicializado)");
        efecom::BindProgram(m_program);
    }

}
}
