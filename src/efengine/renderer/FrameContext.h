#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/SceneLighting.h>

namespace efengine {
namespace scene { class SceneGraph; class Camera; }
namespace renderer {

    class Framebuffer;
    class Renderer;

    // Lo que los pases del frame comparten. Calcado de scene::UpdateContext:
    // referencias y no punteros para lo que no puede faltar ni cambiar de
    // identidad, y UN solo lugar donde un pase publica lo que otro consume.
    //
    // Es lo que reemplaza a la cadena de parametros que se pasaban entre si en
    // Application::RenderScene, donde el orden y las dependencias vivian en
    // comentarios.
    struct FrameContext {
        scene::SceneGraph&   scene;
        const scene::Camera& camera;
        Renderer&            renderer;
        Framebuffer&         sceneFB;
        u32 width  = 0u;
        u32 height = 0u;

        // Los contextos de iluminacion que los pases se pasan entre si. Arranca
        // vacio en cada frame: un contexto del frame anterior con la camara de
        // este es peor que no tener contexto.
        SceneLighting lighting;

        // El prepass del AO ya escribio la profundidad de ESTE frame en el depth
        // de sceneFB: el forward puede dibujar con GL_EQUAL en vez de resolver
        // la visibilidad otra vez. Lo prende AoPass, lo lee ForwardPass.
        //
        // Arranca en false SIEMPRE. Si el prepass no corrio, ese depth tiene la
        // profundidad del FRAME ANTERIOR, y dibujar con GL_EQUAL contra el deja
        // la pantalla vacia.
        bool depthReady = false;
    };

}
}
