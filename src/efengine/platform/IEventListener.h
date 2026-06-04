#pragma once

#include <efengine/core/Types.h>

namespace efengine {
namespace platform {
    class IEventListener {
        public:
            virtual ~IEventListener();

            virtual void OnWindowResize(u32 width, u32 height) {}
            virtual void OnMouseMove(f32 x, f32 y) {}
            virtual void OnMouseButton(i32 button, i32 action, i32 mods) {}
            virtual void OnMouseScroll(f32 xOffset, f32 yOffset) {}// de prueba
    };
}
}