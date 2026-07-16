#pragma once

#include <efengine/core/Types.h>
#include <efengine/renderer/VertexArray.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/Material.h>
#include <efengine/renderer/Shader.h>

#include <string>
#include <unordered_map>

namespace efengine {
namespace renderer {

    // Façade de dibujo. No posee recursos GL → Rule of Zero (sin ctor/dtor/copy/move).
    class Renderer {
        public:
            void Clear(f32 r, f32 g, f32 b, f32 a) const;
            void Draw(const Model& va, const Shader& shader) const;
            void Draw(const Model& model, const const MaterialMap& materials) const;
            void Draw(const VertexArray& va, const Shader& shader) const;
    };

}
}
