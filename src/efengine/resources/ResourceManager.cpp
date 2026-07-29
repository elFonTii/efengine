#include "ResourceManager.h"
#include <efengine/resources/ModelLoader.h>
#include <efengine/resources/FileIO.h>
#include <efengine/resources/ShaderPreprocessor.h>
#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>
#include <utility>

namespace {

    // Resolver de includes: los paths son relativos a assets/shaders/, asi que
    // un shader escribe #include "common/cubeface.glsl" sin importar en que
    // subcarpeta viva el.
    efengine::resources::IncludeResolver ShaderIncludeResolver() {
        return [](const std::string& path) -> std::optional<std::string> {
            const std::string full = "assets/shaders/" + path;
            return efengine::resources::FileIO::ReadText(full.c_str());
        };
    }

    // Lee un shader y le resuelve los includes. nullopt si falla cualquiera de
    // las dos cosas: el error concreto ya lo logueo quien fallo.
    std::optional<std::string> ReadShaderWithIncludes(const char* path) {
        const std::optional<std::string> raw = efengine::resources::FileIO::ReadText(path);
        if (!raw) return std::nullopt;

        std::optional<std::string> spliced =
            efengine::resources::SpliceIncludes(*raw, ShaderIncludeResolver());
        if (!spliced) {
            EF_LOG_ERROR("ResourceManager: fallo el preprocesado de includes de '%s'", path);
            return std::nullopt;
        }
        return spliced;
    }

}

namespace efengine {
namespace resources {
    renderer::Model* ResourceManager::GetModel(const char* path){
        EF_ASSERT(path != null, "ResourceManager::GetModel: Intentando acceder a un directorio nulo");
        
        if (auto it = m_models.find(path); it != m_models.end()) {
            // found
            return &it->second;
        } else {
            // not found, not cached -> load the model.
            auto result = resources::ModelLoader::Load(path);
            if(!result) return null;

            auto [inserted, ok] = m_models.emplace(path, std::move(*result));
            EF_ASSERT(ok, "ResourceManager::GetModel: emplace no insertó tras un miss");

            // insertó, retornar
            return &inserted->second;
        }
    }

    renderer::Texture* ResourceManager::GetTexture(const char* path, renderer::ColorSpace space){
        EF_ASSERT(path != null, "ResourceManager::GetTexture: Intentando acceder a un directorio nulo");
        
        if (auto it = m_textures.find(path); it != m_textures.end()) {
            // found
            if(it->second.space != space) {
                EF_LOG_WARNING("ResourceManager: '%s' ya cacheada con otro ColorSpace, se reutiliza", path);
            }

            return &it->second.texture;
        } else {
            // not found, not cached -> load the texture.
            auto result = renderer::Texture::Create(path, space);
            if(!result) return null;

            auto [inserted, ok] = m_textures.emplace(path, TextureSlot{ std::move(*result), space });
            EF_ASSERT(ok, "ResourceManager::GetTexture: emplace no insertó tras un miss");

            // insertó, retornar
            return &inserted->second.texture;
        }
    }

    renderer::Shader* ResourceManager::GetShader(const char* name, const char* vertPath, const char* fragPath) {
        EF_ASSERT(vertPath != null && fragPath !=null, "ResourceManager::GetShader: Intentando acceder a un directorio nulo");
        EF_ASSERT(name != null, "ResourceManager::GetShader: Intentando almacenar un shader sin nombre");

        if (auto it = m_shaders.find(name); it != m_shaders.end()) {
            // found
            return &it->second;
        } else {
            // cargar vertex y fragment
            auto vertex = ReadShaderWithIncludes(vertPath);
            auto fragment = ReadShaderWithIncludes(fragPath);
            if(!vertex || !fragment) return null;

            // crear shader
            auto result = renderer::Shader::Create(vertex->c_str(), fragment->c_str());
            if(!result) return null;

            auto [inserted, ok] = m_shaders.emplace(name, std::move(*result));
            EF_ASSERT(ok, "ResourceManager::GetShader: emplace no insertó tras un miss");

            return &inserted->second;
        }
    }

    renderer::Shader* ResourceManager::GetComputeShader(const char* name, const char* computePath) {
        EF_ASSERT(computePath != null, "ResourceManager::GetComputeShader: Intentando acceder a un directorio nulo");
        EF_ASSERT(name != null, "ResourceManager::GetComputeShader: Intentando almacenar un shader sin nombre");

        if (auto it = m_shaders.find(name); it != m_shaders.end()) {
            // found
            return &it->second;
        } else {
            // cargar vertex y fragment
            auto compute = ReadShaderWithIncludes(computePath);
            if(!compute) return null;

            // crear compute shader
            auto result = renderer::Shader::CreateCompute(compute->c_str());
            if(!result) return null;

            auto [inserted, ok] = m_shaders.emplace(name, std::move(*result));
            EF_ASSERT(ok, "ResourceManager::GetComputeShader: emplace no insertó tras un miss");

            return &inserted->second;
        }
    }

    // Ejecutado al serializar (guardar) un modelaso
    // RAII nos garantiza que la dirección de memoria del modelo no cambia mientras esté en el unordered_map.
    const std::string* ResourceManager::PathOf(const renderer::Model* model) const {
        if (model == null) return null;

        for (const auto& entry : m_models) {
            if (&entry.second == model) return &entry.first; // compara direcciones de memoria, no contenido. 
        }
        return null;
    }
}
}