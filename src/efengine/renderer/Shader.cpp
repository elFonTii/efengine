#include "Shader.h"

#include <efecom/RHI.h>
#include <utility>
#include <glm/gtc/type_ptr.hpp>

#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>

namespace efengine {
namespace renderer {

    namespace {
        // Compila un stage. Devuelve 0 (y loguea el infolog) si falla.
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

    void Shader::SetInt(const char* name, i32 value) const {
        EF_ASSERT(m_program != 0, "Shader::SetInt: programa vacio (movido o no inicializado)");

        i32 loc = getUniformLocation(name);
        if (loc == -1) return;
        efecom::SetUniformInt(loc, value);
    }

    void Shader::SetFloat(const char* name, f32 value) const {
        EF_ASSERT(m_program != 0, "Shader::SetFloat: programa vacio (movido no inicializado)");

        i32 loc = getUniformLocation(name);
        if (loc == -1) return;
        efecom::SetUniformFloat(loc, value);
    }

    void Shader::SetVec3(const char* name, const glm::vec3& value) const {
        EF_ASSERT(m_program != 0, "Shader::SetVec3: programa vacio (movido no inicializado)");

        i32 uniformLocation = efecom::GetUniformLocation(m_program, name);

        if(uniformLocation == -1) {
            EF_LOG_WARNING("Shader::SetVec3: fallo al obtener la ubicación del uniform: %s", name);
            return;
        }
        efecom::SetUniformVec3(uniformLocation, glm::value_ptr(value));
    }

    void Shader::SetMat4(const char* name, const glm::mat4& value) const {
        EF_ASSERT(m_program != 0, "Shader::SetMat4: programa vacio (movido no inicializado)");

        i32 loc = getUniformLocation(name);
        if (loc == -1) return;
        efecom::SetUniformMat4(loc, glm::value_ptr(value));
    }

    i32 Shader::getUniformLocation(const char* name) const {
        EF_ASSERT(m_program != 0, "Shader:getUniformLocation: programa vacio (movido o no inicializado)");

        auto result = m_uniformCache.find(name); // no retorna valor, retorna un puntero iterador apuntando al mapa encontrado

        // TODO: FFONTANA - ES UN VALIOSO HELPER SEGURISIMO
        // Manera segura para acceder a una busqueda
        if (result != m_uniformCache.end()) {
            int32_t value = result->second;

            return value; // si encuentra, retorna la ubicación cacheada
        } else {
            i32 location = efecom::GetUniformLocation(m_program, name);
            m_uniformCache[name] = location; // lo agrego al caché

            if(location == -1) EF_LOG_WARNING("Shader: uniform no encontrado (omitido): %s", name);
            return location;
        }
    }
}
}
