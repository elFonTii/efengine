#include "AssetCatalog.h"

#include <efengine/resources/FileIO.h>

namespace sandbox {

    void AssetCatalog::EnsureScanned() {
        if (m_scanned) return;
        Rescan();
    }

    void AssetCatalog::Rescan() {
        using efengine::resources::FileIO;
        // Los modelos estan planos en assets/models; las texturas van por set,
        // en subcarpetas, asi que ahi si hace falta recursivo.
        m_models   = FileIO::ListFiles("assets/models",   { ".fbx" },           false);
        m_textures = FileIO::ListFiles("assets/textures", { ".jpg", ".png", "jpeg" },   true);
        // Las escenas estan planas en assets/scenes, como los modelos.
        m_scenes   = FileIO::ListFiles("assets/scenes",   { ".efe" },           false);
        m_scanned  = true;
    }

}
