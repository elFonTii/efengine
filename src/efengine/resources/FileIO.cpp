#include <efengine/resources/FileIO.h>

#include <fstream>
#include <sstream>
#include <filesystem>

namespace efengine {
namespace resources {
    std::optional<std::string> FileIO::ReadText(const char* path) {
        EF_ASSERT(path != null, "FileIO::ReadText: Path nulo");

        std::filesystem::path filePath{path};
        std::ifstream stream{filePath};

        if(!stream.is_open()) {
             EF_LOG_ERROR("FileIO::ReadText: no se pudo abrir '%s'", path);
             return std::nullopt;
        }

        // acumula el buffer en ss y luego se extrae y retorna como un string.
        std::stringstream ss; ss << stream.rdbuf();
        return ss.str();
    }

    /* https://siliz4.github.io/guides/tutorials/2020/05/21/guide-on-binary-files-cpp.html */
    std::optional<std::vector<u8>> ReadBytes(const char* path) {
        // pasos: medir el archivo entero
        // reservar un vector del mismo tamaño
        // leer el archivo de una sola vez

        EF_ASSERT(path != null, "FileIO::ReadBytes: Path nulo");

        std::filesystem::path filePath{path};
        std::ifstream stream{filePath, std::ios::binary | std::ios::ate}; // abre en modo binario, ate se posiciona al final (para leer tamaño)

        if(!stream.is_open()) {
             EF_LOG_ERROR("FileIO::ReadBytes: no se pudo abrir '%s'", path);
             return std::nullopt;
        }

        // 1. medir el tamaño del archivo
        const std::streamsize size = stream.tellg();
        if(size < 0) {
            EF_LOG_ERROR("FileIO::ReadBytes: no se pudo medir el tamaño de '%s'", path);
            return std::nullopt;
        }

        // 2. reservar un vector del mismo tamaño
        std::vector<u8> bytes(static_cast<usize>(size));

        // 3. leer el archivo de una sola vez
        if(size > 0) {
            stream.seekg(0, std::ios::beg); // vuelve el cursor al inicio

            // lectura
            if(!stream.read(reinterpret_cast<char*>(bytes.data()), size)) { // stream.read habla en char*, pero guardamos en u8, reinterpret_cast es para eso
                EF_LOG_ERROR("FileIO::ReadBytes: no se pudo leer '%s'", path);
                return std::nullopt;
            }
        }
        return bytes;
    }
}
}