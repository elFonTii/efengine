#include "efengine/resources/AssimpMaterial.h"

#include <assimp/material.h>

#include <algorithm>
#include <cctype>

namespace efengine {
namespace resources {

    namespace {

        std::string ConBarras(std::string s) {
            std::replace(s.begin(), s.end(), '\\', '/');
            return s;
        }

        bool EsAbsoluta(const std::string& s) {
            if (!s.empty() && s[0] == '/') return true;
            return s.size() >= 2 && s[1] == ':';
        }

        std::string EnMinuscula(std::string s) {
            std::transform(s.begin(), s.end(), s.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        }

        // Posicion del punto de la extension, o npos. Un punto en un directorio
        // ("Tex.v2/X") no cuenta.
        std::size_t PosDeExtension(const std::string& ruta) {
            const std::size_t punto = ruta.find_last_of('.');
            if (punto == std::string::npos) return std::string::npos;

            const std::size_t barra = ruta.find_last_of('/');
            if (barra != std::string::npos && punto < barra) return std::string::npos;

            return punto;
        }

    }

    std::vector<SlotMapping> MapAssimpTextureType(int assimpType) {
        using renderer::ColorSpace;
        using renderer::TextureSlot;

        switch (assimpType) {
            case 1: return { { TextureSlot::Albedo, ColorSpace::sRGB, false } };
            case 2: return { { TextureSlot::Roughness, ColorSpace::Linear, false },
                             { TextureSlot::Metallic,  ColorSpace::Linear, true  } };
            case 4: return { { TextureSlot::Emissive, ColorSpace::sRGB, false } };
            // Muchos exportadores de FBX mandan el normal map por HEIGHT.
            case 5:
            case 6: return { { TextureSlot::Normal, ColorSpace::Linear, false } };
            default: return {};
        }
    }

    std::string RemapTexturePath(const std::string& assimpPath,
                                 const std::string& modelPath) {
        if (assimpPath.empty()) return std::string();

        std::string ruta = ConBarras(assimpPath);

        if (EsAbsoluta(ruta)) {
            const std::size_t barra = ruta.find_last_of('/');
            if (barra != std::string::npos) ruta = ruta.substr(barra + 1);
        }

        const std::size_t punto = PosDeExtension(ruta);
        if (punto != std::string::npos && EnMinuscula(ruta.substr(punto)) == ".dds") {
            ruta = ruta.substr(0, punto) + ".png";
        }

        std::string dir = ConBarras(modelPath);
        const std::size_t corte = dir.find_last_of('/');
        dir = (corte == std::string::npos) ? std::string() : dir.substr(0, corte + 1);

        return dir + ruta;
    }

    std::string MetallicPathFromSpecular(const std::string& specularPngPath) {
        const std::size_t punto = PosDeExtension(specularPngPath);
        if (punto == std::string::npos) return std::string();
        if (EnMinuscula(specularPngPath.substr(punto)) != ".png") return std::string();

        return specularPngPath.substr(0, punto) + "_Metallic.png";
    }

    std::string MaterialNameFromAssimp(const void* aiMaterialPtr) {
        const aiMaterial* mat = static_cast<const aiMaterial*>(aiMaterialPtr);
        aiString nombre;
        if (mat->Get(AI_MATKEY_NAME, nombre) == AI_SUCCESS && nombre.length > 0) {
            return nombre.C_Str();
        }
        return "material_sin_nombre";
    }

    renderer::MaterialDef MaterialDefFromAssimp(const void* aiMaterialPtr,
                                                const std::string& modelPath) {
        const aiMaterial* mat = static_cast<const aiMaterial*>(aiMaterialPtr);

        renderer::MaterialDef def;
        def.shaderName = "pbr";
        def.vertPath   = "assets/shaders/pbr.vert";
        def.fragPath   = "assets/shaders/pbr.frag";

        def.name = MaterialNameFromAssimp(mat);

        for (int tipo = 1; tipo <= AI_TEXTURE_TYPE_MAX; ++tipo) {
            const std::vector<SlotMapping> mapeos = MapAssimpTextureType(tipo);
            if (mapeos.empty()) continue;

            const aiTextureType t = static_cast<aiTextureType>(tipo);
            if (mat->GetTextureCount(t) == 0) continue;

            aiString ruta;
            if (mat->GetTexture(t, 0, &ruta) != AI_SUCCESS) continue;

            const std::string base = RemapTexturePath(ruta.C_Str(), modelPath);
            if (base.empty()) continue;

            for (const SlotMapping& m : mapeos) {
                const std::string ruta2 = m.companionMetallic ? MetallicPathFromSpecular(base) : base;
                if (ruta2.empty()) continue;

                // NORMALS y HEIGHT mapean los dos a Normal: el primero gana.
                const bool repetido = std::any_of(
                    def.textures.begin(), def.textures.end(),
                    [&](const renderer::TextureDef& td) { return td.slot == m.slot; });
                if (repetido) continue;

                def.textures.push_back(renderer::TextureDef{ m.slot, ruta2, m.space });

                if (m.slot == renderer::TextureSlot::Emissive) def.emissiveIntensity = 1.0f;
            }
        }

        // AI_MATKEY_TWOSIDED viene en 0 en los 133 materiales del Bistro, incluidos
        // los 6 que terminan en .DoubleSided: el sufijo del nombre no es un
        // fallback, es el unico camino. Sin el, ese follaje parpadea.
        int twoSided = 0;
        const bool porFlag = mat->Get(AI_MATKEY_TWOSIDED, twoSided) == AI_SUCCESS && twoSided != 0;

        const bool porNombre = def.name.size() >= 12
                            && def.name.compare(def.name.size() - 12, 12, ".DoubleSided") == 0;

        def.doubleSided = porFlag || porNombre;

        return def;
    }

}
}
