#pragma once

#include <string>
#include <vector>

namespace sandbox {

    // Cache de los listados de disco. Escanear dentro de un combo seria un
    // recorrido recursivo de directorios 60 veces por segundo por nada.
    class AssetCatalog {
        public:
            void EnsureScanned();   // escanea la primera vez; despues no hace nada
            void Rescan();          // el boton "Refrescar"

            const std::vector<std::string>& Models()   const { return m_models; }
            const std::vector<std::string>& Textures() const { return m_textures; }

        private:
            std::vector<std::string> m_models;
            std::vector<std::string> m_textures;
            bool m_scanned = false;
    };

}
