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
