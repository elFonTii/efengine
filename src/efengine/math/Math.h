#pragma once

#include "../core/Types.h"

#include <glm/glm.hpp>

#include <optional>

namespace efengine {
namespace math {

    // Rect de destino en pixeles de pantalla. Existe porque con el dockspace de
    // ImGui el area util NO es la ventana entera: es el nodo central, que tiene
    // su propio origen y su propio tamano.
    struct ScreenRect {
        f32 x = 0.0f;
        f32 y = 0.0f;
        f32 w = 0.0f;
        f32 h = 0.0f;
    };

    // Proyecta un punto de mundo a pixeles dentro de rect.
    //
    // Devuelve nullopt si el punto esta detras de la camara o sobre su plano:
    // con w <= 0 la division perspectiva da una posicion plausible pero
    // espejada, y dibujar eso miente sobre donde esta el punto.
    //
    // La Y crece hacia ABAJO (convencion de ImGui), no hacia arriba como en NDC.
    std::optional<glm::vec2> ProjectToScreen(const glm::mat4& viewProj,
                                             const glm::vec3& world,
                                             const ScreenRect& rect);

}
}
