#pragma once
#include <efengine/core/Types.h>

namespace efengine {
namespace renderer  { class Renderer; class VertexArray; class Framebuffer; }
namespace resources { class ResourceManager; }
namespace application {

    // Lo que cualquier pase puede necesitar para construirse.
    //
    // Un struct de referencias y no parametros sueltos: asi la firma de
    // registro es la MISMA para los diez pases, y agregar uno no obliga a
    // cambiar ninguna de las que ya estan.
    //
    // Vive en application y no en renderer porque incluye el ResourceManager,
    // y resources depende de renderer: al reves seria un ciclo.
    struct PassDeps {
        renderer::Renderer&          renderer;
        resources::ResourceManager&  resources;
        renderer::VertexArray&       fullscreenQuad;
        renderer::Framebuffer&       sceneFB;
        u32 width  = 0u;
        u32 height = 0u;
        // Los 4 floats del color de limpieza, que vive en Application. Puntero
        // y no copia: se cambia desde afuera en cualquier momento.
        const f32* clearColor = null;
    };

}
}
