#include "efengine/resources/MaterialBuilder.h"
#include <efengine/core/Log.h>

namespace efengine {
namespace resources {

    std::optional<renderer::Material> BuildMaterial(const renderer::MaterialDef& def,
                                                    ResourceManager& rm) {
        renderer::Shader* shader = rm.GetShader(def.shaderName.c_str(),
                                                def.vertPath.c_str(),
                                                def.fragPath.c_str());
        if (shader == null) {
            EF_LOG_ERROR("BuildMaterial: no se pudo crear el shader '%s' del material '%s'",
                         def.shaderName.c_str(), def.name.c_str());
            return std::nullopt;
        }

        renderer::Material material(shader);

        for (const renderer::TextureDef& tex : def.textures) {
            renderer::Texture* t = rm.GetTexture(tex.path.c_str(), tex.space);
            if (t == null) {
                // Una textura que falta no puede matar la escena, continua sin el mapa
                EF_LOG_WARNING("BuildMaterial: material '%s' sin el mapa de '%s'",
                               def.name.c_str(), tex.path.c_str());
                continue;
            }
            switch (tex.slot) {
                case renderer::TextureSlot::Albedo:    material.SetAlbedoMap(t);    break;
                case renderer::TextureSlot::Normal:    material.SetNormalMap(t);    break;
                case renderer::TextureSlot::AO:        material.SetAOMap(t);        break;
                case renderer::TextureSlot::Roughness: material.SetRoughnessMap(t); break;
                case renderer::TextureSlot::Metallic:  material.SetMetallicMap(t);  break;
                case renderer::TextureSlot::Height:    material.SetHeightMap(t);    break;
                case renderer::TextureSlot::Opacity:   material.SetOpacityMap(t);   break;
                case renderer::TextureSlot::Emissive:  material.SetEmissiveMap(t);  break;
                default:
                    EF_LOG_WARNING("BuildMaterial: slot de textura %u desconocido",
                                   static_cast<u32>(tex.slot));
                    break;
            }
        }

        material.albedoTint  = def.albedoTint;
        material.metallic    = def.metallic;
        material.roughness   = def.roughness;
        material.aoStrength  = def.aoStrength;
        material.heightScale = def.heightScale;
        material.alphaCutoff = def.alphaCutoff;
        material.emissiveTint      = def.emissiveTint;
        material.emissiveIntensity = def.emissiveIntensity;
        material.normalStrength    = def.normalStrength;

        return material;
    }

}
}