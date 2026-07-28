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
        Emissive  = 7,
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

        // v2 del formato. Los defaults dejan el material igual que antes de la Fase 1:
        // emision apagada y normal map sin escalar.
        glm::vec3 emissiveTint      = glm::vec3(1.0f);   // blanco = no tiñe el mapa
        f32       emissiveIntensity = 0.0f;              // 0 = sin emision
        f32       normalStrength    = 1.0f;              // 1 = el mapa tal cual

        // v3 del formato. false = se cullea la cara de atras (lo de siempre);
        // true = se dibujan las dos caras. Para planos, cortinas y mallas de una
        // sola cara cuyo winding no aguanta el culling.
        bool      doubleSided       = false;
    };

}
}
