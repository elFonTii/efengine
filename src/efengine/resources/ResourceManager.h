#pragma once
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/MaterialDef.h>
#include <efengine/core/Types.h>
#include <unordered_map>
#include <string>
#include <vector>

namespace efengine {
namespace resources {
    class ResourceManager {
        public:
            renderer::Model* GetModel(const char* path);

            // Los materiales que declaraba el archivo del modelo. null si el
            // modelo no se pudo cargar. Sale del mismo cache que GetModel: un
            // solo parseo del FBX.
            const std::vector<renderer::MaterialDef>* GetModelMaterials(const char* path);
            renderer::Texture* GetTexture(const char* path, renderer::ColorSpace space = renderer::ColorSpace::Linear);
            renderer::Shader* GetShader(const char* name, const char* vertPath, const char* fragPath);
            renderer::Shader* GetComputeShader(const char* name, const char* computePath);
            const std::string* PathOf(const renderer::Model* model) const; // obtiene el path del modelo para serializar

        private:
            struct TextureSlot { renderer::Texture texture; renderer::ColorSpace space; };

            struct ModelSlot {
                renderer::Model                    model;
                std::vector<renderer::MaterialDef> materials;
            };

            ModelSlot* LoadSlot(const char* path);

            std::unordered_map<std::string, ModelSlot> m_models;
            std::unordered_map<std::string, TextureSlot> m_textures;
            std::unordered_map<std::string, renderer::Shader> m_shaders; // compartido entre vertex/fragment y compute

    };
}
}