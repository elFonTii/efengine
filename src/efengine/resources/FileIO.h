#pragma once

#include <efengine/core/Assert.h>
#include <efengine/core/Types.h>
#include <efengine/core/Log.h>

#include <optional>
#include <string>
#include <vector>


namespace efengine {
namespace resources {
    class FileIO {
       public:
        static std::optional<std::string> ReadText(const char* path);
        static std::optional<std::vector<u8>> ReadBytes(const char* path);

        // Lista archivos de 'dir' filtrando por extension. Devuelve paths con '/'
        // como separador (usables tal cual en GetModel/GetTexture) y ORDENADOS:
        // sin orden estable el combo de ImGui baila entre frames.
        // extensions vacio = todos los archivos. Directorio invalido = vector vacio.
        static std::vector<std::string> ListFiles(const char* dir,
                                                  const std::vector<std::string>& extensions,
                                                  bool recursive);
    };
}
}