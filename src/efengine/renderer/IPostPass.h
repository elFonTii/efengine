#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/RenderTarget.h>

namespace efengine {
namespace renderer {
    class Texture;

    class IPostPass {
        public:
            virtual ~IPostPass() = default;
            // target siempre es valido: la convencion del puntero nulo que
            // significaba "el backbuffer" se fue con RenderTarget::Present().
            virtual void Apply(const Texture& input, const RenderTarget& target) = 0;
            virtual void Resize(u32 width, u32 height) = 0;
    };
}
}
