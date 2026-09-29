#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/SceneLighting.h>
#include <efengine/renderer/FrameView.h>

namespace efengine {
namespace scene { class SceneGraph; class Camera; }
namespace renderer {

    class Framebuffer;
    class Texture;
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
        // La camara del frame. Unica fuente de view/projection: ver FrameView.h.
        FrameView view;

        // Los contextos de iluminacion que los pases se pasan entre si. Arranca
        // vacio en cada frame: un contexto del frame anterior con la camara de
        // este es peor que no tener contexto.
        SceneLighting lighting;

        // El DepthPrepass ya escribio la profundidad de ESTE frame en el depth
        // de sceneFB: el forward puede dibujar con GL_EQUAL en vez de resolver
        // la visibilidad otra vez. Lo prende DepthPrepass, lo lee ForwardPass.
        //
        // Arranca en false SIEMPRE. Si el prepass no corrio, ese depth tiene la
        // profundidad del FRAME ANTERIOR, y dibujar con GL_EQUAL contra el deja
        // la pantalla vacia.
        bool depthReady = false;

        // Los dos del DepthPrepass de ESTE frame; null si no corrio.
        const Texture* depthNormal = null;   // xyz = normal de vista, w = viewZ lineal
        const Texture* depth       = null;   // D32F de sceneFB
    };

}
}
