#include "efengine/renderer/Vertex.h"

namespace efengine {
namespace renderer {

    VertexLayout StandardVertexLayout() {
        VertexLayout layout;

        layout.Push(renderer::ShaderDataType::Float3);
        layout.Push(renderer::ShaderDataType::Float3);
        layout.Push(renderer::ShaderDataType::Float2);
        layout.Push(renderer::ShaderDataType::Float3);

        return layout;
    }

}
}