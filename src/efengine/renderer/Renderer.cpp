#include "Renderer.h"

#include <glad/gl.h>

#include <efengine/core/Assert.h>

namespace efengine {
namespace renderer {

    void Renderer::Clear(f32 r, f32 g, f32 b, f32 a) const {
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void Renderer::Draw(const VertexArray& va, const Shader& shader) const {
        EF_ASSERT(va.vertexCount() > 0, "Renderer::Draw: VertexArray sin vertices");

        shader.Bind();
        va.Bind();
        
        if(va.hasIndexBuffer()) {
            glDrawElements(GL_TRIANGLES, (GLsizei)va.indexCount(), GL_UNSIGNED_INT, nullptr);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)va.vertexCount());
        }

        glBindVertexArray(0);
        glUseProgram(0);
    }

}
}
