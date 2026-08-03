#include <efengine/resources/FileIO.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <system_error>

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

        std::stringstream ss; ss << stream.rdbuf();
        return ss.str();
    }

    /* https://siliz4.github.io/guides/tutorials/2020/05/21/guide-on-binary-files-cpp.html */
    std::optional<std::vector<u8>> FileIO::ReadBytes(const char* path) {
        EF_ASSERT(path != null, "FileIO::ReadBytes: Path nulo");

        std::filesystem::path filePath{path};
        std::ifstream stream{filePath, std::ios::binary | std::ios::ate}; // abre en modo binario, ate se posiciona al final (para leer tamaño)

        if(!stream.is_open()) {
             EF_LOG_ERROR("FileIO::ReadBytes: no se pudo abrir '%s'", path);
             return std::nullopt;
        }

        const std::streamsize size = stream.tellg();
        if(size < 0) {
            EF_LOG_ERROR("FileIO::ReadBytes: no se pudo medir el tamaño de '%s'", path);
            return std::nullopt;
        }

        std::vector<u8> bytes(static_cast<usize>(size));

        if(size > 0) {
            stream.seekg(0, std::ios::beg);

            if(!stream.read(reinterpret_cast<char*>(bytes.data()), size)) { // stream.read habla en char*, pero guardamos en u8, reinterpret_cast es para eso
                EF_LOG_ERROR("FileIO::ReadBytes: no se pudo leer '%s'", path);
                return std::nullopt;
            }
        }
        return bytes;
    }

    namespace {
        // ".PNG", "PNG" y "png" tienen que matchear igual: normaliza a minusculas con punto.
        std::string normalizarExt(const std::string& ext) {
            std::string out = ext;
            if (!out.empty() && out.front() != '.') out.insert(out.begin(), '.');
            for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return out;
        }
    }

    std::vector<std::string> FileIO::ListFiles(const char* dir,
                                               const std::vector<std::string>& extensions,
                                               bool recursive) {
        EF_ASSERT(dir != null, "FileIO::ListFiles: Path nulo");

        std::vector<std::string> out;

        std::error_code ec;
        const std::filesystem::path base{dir};
        if (!std::filesystem::is_directory(base, ec) || ec) {
            EF_LOG_WARNING("FileIO::ListFiles: '%s' no es un directorio", dir);
            return out;
        }

        std::vector<std::string> filtros;
        filtros.reserve(extensions.size());
        for (const std::string& e : extensions) filtros.push_back(normalizarExt(e));

        auto agregar = [&](const std::filesystem::path& p) {
            if (!filtros.empty()) {
                const std::string ext = normalizarExt(p.extension().string());
                if (std::find(filtros.begin(), filtros.end(), ext) == filtros.end()) return;
            }
            out.push_back(p.generic_string());   // '/' siempre, tambien en Windows
        };

        if (recursive) {
            for (std::filesystem::recursive_directory_iterator it(base, ec), end;
                 it != end; it.increment(ec)) {
                if (ec) break;
                if (it->is_regular_file(ec)) agregar(it->path());
            }
        } else {
            for (std::filesystem::directory_iterator it(base, ec), end;
                 it != end; it.increment(ec)) {
                if (ec) break;
                if (it->is_regular_file(ec)) agregar(it->path());
            }
        }

        std::sort(out.begin(), out.end());
        return out;
    }
}
}