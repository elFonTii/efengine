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
}
}