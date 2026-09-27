#pragma once
#include <efengine/core/Types.h>

#include <glm/glm.hpp>

namespace efengine {
namespace renderer {

    class VertexArray;
    class Material;

    // Un draw ya resuelto: la geometria, su material, y el objeto al que
    // pertenece. Punteros y no valores porque los tres viven en la escena y en
    // los recursos, y esta lista se rearma cada frame.
    struct BatchDraw {
        const VertexArray* va       = null;
        const Material*    material = null;
        const glm::mat4*   world    = null;
    };

    // Lo que emitio un SubmitBatch. Sale al panel: sin estos dos numeros no hay
    // forma de distinguir "el culling anda" de "el culling no descarta nada".
    struct BatchStats {
        u32 draws           = 0u;
        u32 materialUploads = 0u;
    };

}
}
