#pragma once
#include <efengine/core/Types.h>

namespace efengine {
namespace renderer {
    class Texture; 
    class Framebuffer; 

    class IPostPass {
        public:
            virtual ~IPostPass() = default;
            virtual void Apply(const Texture& input, const Framebuffer* target) = 0;
            virtual void Resize(u32 width, u32 height) = 0;

    };
}
}