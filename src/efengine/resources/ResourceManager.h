#pragma once
#include <efengine/renderer/Model.h>
#include <efengine/core/Types.h>
#include <unordered_map>
#include <string>

namespace efengine {
namespace resources {
    class ResourceManager {
        public:
            renderer::Model* GetModel(const char* path);
            
        private:
            std::unordered_map<std::string, renderer::Model> m_models;
    };
}
}