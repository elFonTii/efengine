#include "efengine/scene/NodeCamera.h"

#include <efengine/scene/Camera.h>
#include <efengine/scene/SceneGraph.h>

namespace efengine {
namespace scene {

    bool ApplyNodeCamera(Camera& out, const SceneGraph& graph, NodeHandle node, f32 aspect) {
        const Node* n = graph.TryGet(node);
        if (n == null || !n->camera) return false;

        const CameraAttachment& att = *n->camera;

        // El world se resuelve AHORA por la cadena de padres: el cache todavia
        // esta sucio, lo refresca RenderScene mas tarde en el frame.
        out.SetFromWorld(graph.WorldMatrixOf(node));
        out.SetFov(att.fovDeg);
        out.SetClipPlanes(att.nearPlane, att.farPlane);
        out.SetExposure(att.exposure);
        out.SetAspect(aspect);
        return true;
    }

}
}
