#pragma once

#include <efengine/core/Types.h>
#include <efengine/renderer/Cubemap.h>
#include <efengine/renderer/Shader.h>

#include <optional>

namespace efengine {
namespace renderer {

    // Precómputo de ibl: proyecta un .hdr a un cubemap de entorno y lo convoluciona
    // a un mapa de irradiancia difusa (el resultado completo del precómputo IBL) (ro5).
    // Sigue siendo ro5 válido: ambos miembros son Cubemap RAII, el move/destroy es implícito.
    class Environment {
        public:
            static std::optional<Environment> Create(const char* hdrPath,
                                                     const Shader& equirectToCube,
                                                     const Shader& irradianceConvolve,
                                                     u32 faceSize = 512,
                                                     u32 irradianceSize = 32);

            const Cubemap& env() const { return m_env; }
            const Cubemap& irradiance() const { return m_irradiance; }

        private:
            Environment(Cubemap env, Cubemap irradiance);

            Cubemap m_env;
            Cubemap m_irradiance;
    };

}
}