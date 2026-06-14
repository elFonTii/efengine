#include "efengine/renderer/Material.h"
#include "efengine/renderer/Texture.h"
#include "efengine/renderer/Shader.h"
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



    void Material::bindMap(const Shader& shader, const Texture* texture, u32 unit, const char* mapUniform, const char* hasUniform) {
        if(texture == null) {
            shader.SetInt(hasUniform, 0);
            return;
        }
         texture->Bind(unit); shader.SetInt(mapUniform, unit); shader.SetInt(hasUniform, 1);
    }

    void Material::Bind() const {
        EF_ASSERT(m_shader != null, "Material::Bind: shader nulo");
        m_shader->Bind();
        m_shader->SetVec3("uAlbedoTint", albedoTint);
        m_shader->SetFloat("uMetallic", metallic);
        m_shader->SetFloat("uRoughness", roughness);
        m_shader->SetFloat("uAOStrength", aoStrength);
        m_shader->SetFloat("uHeightScale", heightScale);
        m_shader->SetFloat("uAlphaCutoff", alphaCutoff);
        bindMap(*m_shader, m_albedo,    0, "uAlbedoMap",    "uHasAlbedoMap");
        bindMap(*m_shader, m_normal,    1, "uNormalMap",    "uHasNormalMap");
        bindMap(*m_shader, m_ao,        2, "uAOMap",        "uHasAOMap");
        bindMap(*m_shader, m_roughness, 3, "uRoughnessMap", "uHasRoughnessMap");
        bindMap(*m_shader, m_metallic,  4, "uMetallicMap",  "uHasMetallicMap");
        bindMap(*m_shader, m_height,    5, "uHeightMap",    "uHasHeightMap");
        bindMap(*m_shader, m_opacity,   6, "uOpacityMap",   "uHasOpacityMap");
    }
}
}