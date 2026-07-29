#pragma once
#include <efengine/core/Types.h>

#include <functional>
#include <optional>
#include <string>

namespace efengine {
namespace resources {

    // Resuelve un path de include a su contenido. Devuelve nullopt si no existe.
    // Se inyecta para que SpliceIncludes sea testeable sin filesystem.
    using IncludeResolver = std::function<std::optional<std::string>(const std::string& path)>;

    // Profundidad maxima de anidamiento. pbr.frag -> ddgi/common.glsl ->
    // common/cubeface.glsl son 2 niveles; 4 deja margen sin permitir un ciclo
    // infinito si alguien se incluye a si mismo indirectamente.
    inline constexpr u32 kMaxIncludeDepth = 4u;

    // Reemplaza cada linea '#include "path"' por el contenido que devuelve el
    // resolver, recursivamente. Semantica include-once: un path ya incluido se
    // reemplaza por una linea vacia en vez de duplicarse.
    //
    // Solo se reconoce el include cuando '#include' arranca la linea (se permiten
    // espacios antes). Un '#include' dentro de un comentario o despues de codigo
    // se deja intacto: no somos un preprocesador de C, y meterse a parsear
    // comentarios es mas riesgo que beneficio.
    //
    // Devuelve nullopt si un include no resuelve o si se excede kMaxIncludeDepth;
    // el caller loguea y falla la carga del shader (recuperable).
    std::optional<std::string> SpliceIncludes(const std::string& source,
                                             const IncludeResolver& resolver);

}
}
