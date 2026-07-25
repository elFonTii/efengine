#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/Texture.h>   // ColorSpace

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace efengine {
namespace renderer {

    // Que mapa de un material. Los valores son PARTE DEL FORMATO .efe:
    // no reordenar ni renumerar sin subir kCurrentVersion.
    enum class TextureSlot : u32 {
        Albedo    = 0,
        Normal    = 1,
        AO        = 2,
        Roughness = 3,
        Metallic  = 4,
        Height    = 5,
        Opacity   = 6,
    };

    struct TextureDef {
        TextureSlot slot  = TextureSlot::Albedo;
        std::string path;
        ColorSpace  space = ColorSpace::Linear;
    };

    // Descripcion serializable de un material. Material en runtime solo
    // guarda punteros a Shader/Texture y no recuerda de donde vinieron; el def SI, y
    // es por eso la fuente de verdad para guardar.
    struct MaterialDef {
        std::string name;
        std::string shaderName;
        std::string vertPath;
        std::string fragPath;
        std::vector<TextureDef> textures;

        glm::vec3 albedoTint  = glm::vec3(1.0f);
        f32       metallic    = 0.5f;
        f32       roughness   = 1.0f;
        f32       aoStrength  = 0.5f;
        f32       heightScale = 0.05f;
        f32       alphaCutoff = 0.5f;
    };

}
}
