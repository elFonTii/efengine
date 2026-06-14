#include <doctest/doctest.h>
#include <efengine/resources/FileIO.h>

#include <fstream>
#include <string>
#include <cstdio>

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
