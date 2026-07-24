#pragma once

#include <efengine/core/Types.h>
#include <efengine/renderer/Cubemap.h>
#include <efengine/renderer/Shader.h>

#include <optional>

namespace efengine {
namespace renderer {

    // Precómputo de ibl, proyecta un .hdr a un cubemap de entorno con compute shader (ro5)
    // en este caso puede quedar como ro5 porque el unico miembro (Cubemap) ya usa RAII, queda implicito.
    class Environment {
        public:
            static std::optional<Environment> Create(const char* hdrPath,
                                                     const Shader& equirectToCube,
                                                     u32 faceSize = 512);

            const Cubemap& env() const { return m_env; }

        private:
            explicit Environment(Cubemap env);

            Cubemap m_env;
    };

}
}