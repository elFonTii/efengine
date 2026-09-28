#pragma once
#include <efengine/core/Types.h>

namespace sandbox {

    struct EditorContext;

    struct LightStressDesc {
        u32 lights = 1024u;
        f32 size   = 200.0f;   // lado del piso, en metros
    };

    // Escena de medicion del clustered: piso, columnas cada 20 m y 'lights' luces
    // con semilla fija (70% point, 30% spot hacia abajo). Es de medicion, no de
    // autoria: el behavior que las gira no se registra y se pierde al guardar.
    void BuildLightStressScene(EditorContext& ctx, const LightStressDesc& desc);

}
