#pragma once
#include <efengine/renderer/Model.h>
#include <optional>

namespace efengine {
namespace resources {
    class ModelLoader {
        public:
            static std::optional<renderer::Model> Load(const char* path);
    };
}
}