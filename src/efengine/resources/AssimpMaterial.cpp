#include "efengine/resources/AssimpMaterial.h"

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

}
}
