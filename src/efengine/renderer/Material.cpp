#include "efengine/renderer/Material.h"
#include "efengine/renderer/Texture.h"
#include "efengine/renderer/Shader.h"
#include "efengine/renderer/MaterialDef.h"
#include "efengine/core/Assert.h"
#include "efengine/core/Log.h"

namespace efengine {
namespace renderer {
    void Material::SetAlbedoMap(const Texture* texture) { m_albedo = texture; }
    void Material::SetNormalMap(const Texture* texture) { m_normal = texture; }
    void Material::SetAOMap(const Texture* texture) { m_ao = texture; }
    void Material::SetRoughnessMap(const Texture* texture) { m_roughness = texture; }
    void Material::SetMetallicMap(const Texture* texture) { m_metallic = texture; }
    void Material::SetHeightMap(const Texture* texture) { m_height = texture; }
    void Material::SetOpacityMap(const Texture* texture) { m_opacity = texture; }
    void Material::SetEmissiveMap(const Texture* texture) { m_emissive = texture; }



    namespace {
        // Bindea la textura a la unidad si esta. TextureSlot es la unica fuente de
        // verdad de la correspondencia slot <-> unidad <-> bit.
        void bindSiEsta(const Texture* texture, TextureSlot slot) {
            if (texture == null) return;
            texture->Bind(static_cast<u32>(slot));
        }

        u32 bitSiEsta(const Texture* texture, TextureSlot slot) {
            return (texture != null) ? (1u << static_cast<u32>(slot)) : 0u;
        }
    }

    void Material::BindTextures() const {
        // La unidad de cada mapa es el valor de su TextureSlot; de 8 en adelante
        // son los mapas de frame (shadow, irradiancia, prefiltrado, LUT).
        bindSiEsta(m_albedo,    TextureSlot::Albedo);
        bindSiEsta(m_normal,    TextureSlot::Normal);
        bindSiEsta(m_ao,        TextureSlot::AO);
        bindSiEsta(m_roughness, TextureSlot::Roughness);
        bindSiEsta(m_metallic,  TextureSlot::Metallic);
        bindSiEsta(m_height,    TextureSlot::Height);
        bindSiEsta(m_opacity,   TextureSlot::Opacity);
        bindSiEsta(m_emissive,  TextureSlot::Emissive);
        // Un mapa ausente no desbindea su unidad: el shader nunca la muestrea
        // porque el bit correspondiente del mapMask esta en cero.
    }

    MaterialBlock Material::ToBlock() const {
        MaterialBlock b {};
        b.albedoTint   = glm::vec4(albedoTint, 0.0f);
        b.emissiveTint = glm::vec4(emissiveTint, 0.0f);
        b.scalars0     = glm::vec4(metallic, roughness, aoStrength, heightScale);
        b.scalars1     = glm::vec4(alphaCutoff, emissiveIntensity, normalStrength, 0.0f);

        const u32 mask = bitSiEsta(m_albedo,    TextureSlot::Albedo)
                       | bitSiEsta(m_normal,    TextureSlot::Normal)
                       | bitSiEsta(m_ao,        TextureSlot::AO)
                       | bitSiEsta(m_roughness, TextureSlot::Roughness)
                       | bitSiEsta(m_metallic,  TextureSlot::Metallic)
                       | bitSiEsta(m_height,    TextureSlot::Height)
                       | bitSiEsta(m_opacity,   TextureSlot::Opacity)
                       | bitSiEsta(m_emissive,  TextureSlot::Emissive);
        b.mapMask = glm::uvec4(mask, 0u, 0u, 0u);

        return b;
    }

    MaterialMap MakeUniformMaterialMap(const std::vector<std::string>& submeshNames,
                                       const Material* material) {
        MaterialMap out;
        if (material == null) return out;
        for (const std::string& name : submeshNames) out[name] = material;
        return out;
    }
}
}