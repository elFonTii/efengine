#pragma once

#include <efengine/core/Types.h>
#include <efengine/renderer/Cubemap.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/Shader.h>

#include <optional>

namespace efengine {
namespace renderer {

    struct EnvironmentDesc { const char* hdrPath; u32 faceSize; u32 irradianceSize; u32 prefilterSize; u32 prefilterMips; u32 lutSize; };
    struct EnvironmentShaders Create(const EnvironmentDesc&, const EnvironmentShaders&);

    // Precómputo de ibl: proyecta un .hdr a un cubemap de entorno y lo convoluciona
    // a un mapa de irradiancia difusa (el resultado completo del precómputo IBL) (ro5).
    // Sigue siendo ro5 válido: ambos miembros son Cubemap RAII, el move/destroy es implícito.
    class Environment {
        public:
            static std::optional<Environment> Create(const EnvironmentDesc&, const EnvironmentShaders&);

            const Cubemap& env() const { return m_env; }
            const Cubemap& irradiance() const { return m_irradiance; }
            const Texture& brdfLut() const { return m_brdfLut; }

        private:
            Environment(Cubemap env, Cubemap irradiance);

            Cubemap m_env;
            Cubemap m_irradiance;
            Texture m_brdfLut;
    };

}
}