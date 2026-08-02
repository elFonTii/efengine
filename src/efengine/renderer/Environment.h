#pragma once

#include <efengine/core/Types.h>
#include <efengine/renderer/Cubemap.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/Shader.h>

#include <optional>

namespace efengine {
namespace renderer {

    // Parametros del precomputo IBL. Todo lo que hoy son argumentos sueltos de Create.
    struct EnvironmentDesc {
        const char* hdrPath        = nullptr;
        u32         faceSize       = 512;
        u32         irradianceSize = 32;
        u32         prefilterSize  = 128;   // cara del mip 0 del prefiltrado
        u32         prefilterMips  = 5;     // 5 niveles => roughness 0, .25, .5, .75, 1
        u32         lutSize        = 512;
    };

    // Los computes que hacen el precomputo. Punteros no-dueños: viven en el
    // ResourceManager. Create falla si alguno es null.
    struct EnvironmentShaders {
        const Shader* equirectToCube     = nullptr;
        const Shader* irradianceConvolve = nullptr;
        const Shader* prefilterGGX       = nullptr;
        const Shader* brdfLut            = nullptr;
    };

    // Precómputo de ibl: proyecta un .hdr a un cubemap de entorno y lo convoluciona
    // a un mapa de irradiancia difusa (el resultado completo del precómputo IBL) (ro5).
    // Sigue siendo ro5 válido: los tres miembros son RAII con move (Cubemap y Texture),
    // el move/destroy es implícito.
    class Environment {
        public:
            static std::optional<Environment> Create(const EnvironmentDesc& desc, const EnvironmentShaders& shaders);

            const Cubemap& env() const { return m_env; }
            const Cubemap& irradiance() const { return m_irradiance; }
            const Cubemap& prefiltered() const { return m_prefiltered; }
            const Texture& brdfLut() const { return m_brdfLut; }

            // LOD maximo del prefiltrado: en el shader la rugosidad se mapea a
            // roughness * prefilterMaxLod(). Con 5 mips son 4.
            f32 prefilterMaxLod() const { return m_prefilterMaxLod; }

        private:
            Environment(Cubemap env, Cubemap irradiance, Cubemap prefiltered, Texture brdfLut,
                        f32 prefilterMaxLod);

            Cubemap m_env;
            Cubemap m_irradiance;
            Cubemap m_prefiltered;
            Texture m_brdfLut;
            f32     m_prefilterMaxLod = 0.0f;
    };

}
}