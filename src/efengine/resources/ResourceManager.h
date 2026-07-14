#pragma once
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/Model.h>
#include <efengine/core/Types.h>
#include <unordered_map>
#include <string>

namespace efengine {
namespace resources {
    class ResourceManager {
        public:
            renderer::Model* GetModel(const char* path);
            renderer::Texture* GetTexture(const char* path, renderer::ColorSpace space = renderer::ColorSpace::Linear);

        private:
            struct TextureSlot { renderer::Texture texture; renderer::ColorSpace space; };
            
            std::unordered_map<std::string, renderer::Model> m_models;
            std::unordered_map<std::string, TextureSlot> m_textures;
    };
}
}