#include <doctest/doctest.h>
#include <efengine/resources/FileIO.h>

#include <fstream>
#include <string>
#include <cstdio>
#include <vector>

TEST_CASE("FileIO::ReadText: check de un archivo conocido") {
    const char* path = "file_io_test.tmp";
    const std::string contenido = "#version 330 core\nvoid main() {}\n";

    {
        std::ofstream out(path);
        REQUIRE(out.is_open());
        out << contenido;
    }

    auto resultado = efengine::resources::FileIO::ReadText(path);

    CHECK(resultado.has_value());
    CHECK(*resultado == contenido);

    std::remove(path);
}

TEST_CASE("FileIO::ReadText: path inexistente devuelve nullopt") {
    auto resultado = efengine::resources::FileIO::ReadText("no_existe_12345.txt");
    CHECK_FALSE(resultado.has_value());
}

TEST_CASE("FileIO::ReadBytes ruta inexistente -> nullopt") {
    auto r = efengine::resources::FileIO::ReadBytes("assets/no_existe_xyz.efe");
    CHECK_FALSE(r.has_value());
}

TEST_CASE("FileIO::ReadBytes lee binario exacto, incluidos ceros") {
    const char* path = "efengine_readbytes_test.bin";
    // Los ceros intermedios son el punto: ReadText los cortaria, ReadBytes no.
    const unsigned char datos[] = { 0x00u, 0x01u, 0xFFu, 0x00u, 0x7Fu, 0x80u, 0x00u };
    {
        std::ofstream out(path, std::ios::binary);
        REQUIRE(out.is_open());
        out.write(reinterpret_cast<const char*>(datos), sizeof(datos));
    }

    auto r = efengine::resources::FileIO::ReadBytes(path);
    std::remove(path);

    REQUIRE(r.has_value());
    REQUIRE(r->size() == sizeof(datos));
    for (usize i = 0u; i < sizeof(datos); ++i) {
        CHECK((*r)[i] == datos[i]);
    }
}

TEST_CASE("FileIO::ReadBytes archivo vacio -> vector vacio, no nullopt") {
    const char* path = "efengine_readbytes_vacio.bin";
    { std::ofstream out(path, std::ios::binary); REQUIRE(out.is_open()); }

    auto r = efengine::resources::FileIO::ReadBytes(path);
    std::remove(path);

    REQUIRE(r.has_value());     // el archivo existe: leerlo funciono
    CHECK(r->empty());          // simplemente no tiene bytes
}

// ---------------------------------------------------------------------------

#include <filesystem>
#include <system_error>

namespace {
    // Arbolito temporal que se borra solo. El test NO puede mirar assets/ real:
    // ctest corre con el cwd en build/ y no hay garantia de que exista.
    struct DirTemporal {
        std::filesystem::path root;

        explicit DirTemporal(const char* nombre)
            : root(std::filesystem::temp_directory_path() / nombre) {
            std::error_code ec;
            std::filesystem::remove_all(root, ec);
            std::filesystem::create_directories(root / "sub");
        }
        ~DirTemporal() {
            std::error_code ec;
            std::filesystem::remove_all(root, ec);
        }

        void Archivo(const char* rel) const {
            std::ofstream out(root / rel);
            out << "x";
        }
        std::string Path() const { return root.generic_string(); }
    };
}

TEST_CASE("FileIO::ListFiles filtra por extension sin distinguir mayusculas") {
    DirTemporal d("efengine_listfiles_ext");
    d.Archivo("b.png");
    d.Archivo("a.PNG");
    d.Archivo("c.txt");

    const auto r = efengine::resources::FileIO::ListFiles(d.Path().c_str(), {"png"}, false);

    REQUIRE(r.size() == 2u);
    CHECK(r[0] == d.Path() + "/a.PNG");   // alfabetico, no orden de creacion
    CHECK(r[1] == d.Path() + "/b.png");
}

TEST_CASE("FileIO::ListFiles: recursive decide si entra a los subdirectorios") {
    DirTemporal d("efengine_listfiles_rec");
    d.Archivo("raiz.fbx");
    d.Archivo("sub/hijo.fbx");

    const auto plano = efengine::resources::FileIO::ListFiles(d.Path().c_str(), {".fbx"}, false);
    REQUIRE(plano.size() == 1u);
    CHECK(plano[0] == d.Path() + "/raiz.fbx");

    const auto hondo = efengine::resources::FileIO::ListFiles(d.Path().c_str(), {".fbx"}, true);
    REQUIRE(hondo.size() == 2u);
    CHECK(hondo[0] == d.Path() + "/raiz.fbx");        // 'r' < 's': el orden es estable
    CHECK(hondo[1] == d.Path() + "/sub/hijo.fbx");
}

TEST_CASE("FileIO::ListFiles con lista de extensiones vacia devuelve todos los archivos") {
    DirTemporal d("efengine_listfiles_todos");
    d.Archivo("uno.png");
    d.Archivo("dos.txt");

    const auto r = efengine::resources::FileIO::ListFiles(d.Path().c_str(), {}, false);
    CHECK(r.size() == 2u);
}

TEST_CASE("FileIO::ListFiles directorio inexistente -> vector vacio, sin excepcion") {
    const auto r = efengine::resources::FileIO::ListFiles("no_existe_dir_12345", {".png"}, false);
    CHECK(r.empty());
}
