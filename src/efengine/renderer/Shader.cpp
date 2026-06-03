#include "Shader.h"

#include <glad/gl.h>
#include <utility>
#include <glm/gtc/type_ptr.hpp>

#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>

namespace efengine {
namespace renderer {

    namespace {
        // Compila un stage. Devuelve 0 (y loguea el infolog) si falla.
        u32 compilar_stage(GLenum tipo, const char* src) {
            const u32 stage = glCreateShader(tipo);
            EF_ASSERT(stage != 0, "compilar_stage: glCreateShader devolvio 0 (sin contexto GL o tipo invalido)");
            glShaderSource(stage, 1, &src, null);
            glCompileShader(stage);

            GLint ok = GL_FALSE;
            glGetShaderiv(stage, GL_COMPILE_STATUS, &ok);
            if (ok != GL_TRUE) {
                char log[512] = {};
                glGetShaderInfoLog(stage, sizeof(log), null, log);
                EF_LOG_ERROR("Shader: fallo al compilar stage: %s", log);
                glDeleteShader(stage);
                return 0;
            }
            return stage;
        }
    }

    std::optional<Shader> Shader::Create(const char* vertexSrc, const char* fragmentSrc) {
        EF_ASSERT(vertexSrc != null, "Shader::Create: vertexSrc no puede ser null");
        EF_ASSERT(fragmentSrc != null, "Shader::Create: fragmentSrc no puede ser null");

        const u32 vs = compilar_stage(GL_VERTEX_SHADER, vertexSrc);
        if (vs == 0) {
            return std::nullopt;
        }

        const u32 fs = compilar_stage(GL_FRAGMENT_SHADER, fragmentSrc);
        if (fs == 0) {
            glDeleteShader(vs);
            return std::nullopt;
        }

        const u32 program = glCreateProgram();
        EF_ASSERT(program != 0, "Shader::Create: glCreateProgram devolvio 0 (sin contexto GL)");
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);

        // Los stages ya no se necesitan tras linkear.
        glDeleteShader(vs);
        glDeleteShader(fs);

        GLint ok = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &ok);
        if (ok != GL_TRUE) {
            char log[512] = {};
            glGetProgramInfoLog(program, sizeof(log), null, log);
            EF_LOG_ERROR("Shader: fallo al linkear el programa: %s", log);
            glDeleteProgram(program);
            return std::nullopt;
        }

        return Shader(program);
    }

    Shader::Shader(u32 program) : m_program(program) {}

    Shader::~Shader() {
        if (m_program != 0) {
            glDeleteProgram(m_program);
        }
    }

    Shader::Shader(Shader&& other) noexcept
        : m_program(std::exchange(other.m_program, 0)) {}

    Shader& Shader::operator=(Shader&& other) noexcept {
        if (this != &other) {
            if (m_program != 0) {
                glDeleteProgram(m_program);
            }
            m_program = std::exchange(other.m_program, 0);
        }
        return *this;
    }

    void Shader::Bind() const {
        EF_ASSERT(m_program != 0, "Shader::Bind: programa vacio (movido o no inicializado)");
        glUseProgram(m_program);
    }

    void Shader::SetInt(const char* name, i32 value) const {
        EF_ASSERT(m_program != 0, "Shader::SetInt: programa vacio (movido o no inicializado)");
        
        i32 uniformLocation = glGetUniformLocation(m_program, name);

        if(uniformLocation == -1) {
            EF_LOG_WARNING("Shader::SetInt: fallo al obtener la ubicación del uniform: %s", name);
            return;
        }

        glUniform1i(uniformLocation, value);
    }

    void Shader::SetFloat(const char* name, f32 value) const {
        EF_ASSERT(m_program != 0, "Shader::SetFloat: programa vacio (movido no inicializado)");

        i32 uniformLocation = glGetUniformLocation(m_program, name);

        if(uniformLocation == -1) {
            EF_LOG_WARNING("Shader::SetFloat: fallo al obtener la ubicación del uniform: %s", name);
            return;
        }
        glUniform1f(uniformLocation, value);
    }

    void Shader::SetVec3(const char* name, const glm::vec3& value) const {
        EF_ASSERT(m_program != 0, "Shader::SetVec3: programa vacio (movido no inicializado)");

        i32 uniformLocation = glGetUniformLocation(m_program, name);

        if(uniformLocation == -1) {
            EF_LOG_WARNING("Shader::SetVec3: fallo al obtener la ubicación del uniform: %s", name);
            return;
        }
        glUniform3fv(uniformLocation, 1, glm::value_ptr(value));
    }

    void Shader::SetMat4(const char* name, const glm::mat4& value) const {
        EF_ASSERT(m_program != 0, "Shader::SetMat4: programa vacio (movido no inicializado)");

        i32 uniformLocation = glGetUniformLocation(m_program, name);

        if(uniformLocation == -1) {
            EF_LOG_WARNING("Shader::SetMat4: fallo al obtener la ubicación del uniform: %s", name);
            return;
        }
        //                                     No transponer
        glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(value));
    }
}
}
