#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/Bounds.h>
#include <glm/glm.hpp>

namespace efengine {
namespace renderer {

    // Matriz light-space de una luz direccional: ortho * lookAt.
    // 'center' es el punto que la caja ortográfica encuadra (origen de escena
    // para una cascada). 'distance' es cuán atrás se ubica el ojo de la luz a lo
    // largo de -direction. Función pura: sin estado ni llamadas GL.
    glm::mat4 ComputeDirectionalLightMatrix(
        const glm::vec3& direction,
        const glm::vec3& center,
        f32 orthoHalfSize,
        f32 distance,
        f32 nearPlane,
        f32 farPlane);

    // Encuadre de la caja ortográfica, derivado de la escena en vez de tuneado
    // a mano.
    //
    // depthRange es el dato que importa afuera: el bias de sombra se compara
    // contra profundidad NDC [0,1], así que un bias de B vale B * depthRange
    // METROS de holgura a lo largo del rayo de luz. Con un encuadre suelto ese
    // factor se dispara y el bias deja de ser dialable — un bias grande despega
    // la sombra del punto de contacto y abre una banda de luz en cada arista
    // entre paredes (peter-panning), y uno chico no tapa el acné del PCF.
    struct DirectionalLightFit {
        glm::mat4 matrix        { 1.0f };
        f32       orthoHalfSize { 1.0f };
        f32       depthRange    { 1.0f };   // far - near, en metros
    };

    // Ajusta la caja a la esfera que contiene 'bounds', más 'padding' metros de
    // aire. Va por la esfera y no por la AABB a propósito: así el encuadre es
    // invariante a la dirección de la luz, y girar el sol no cambia ni el
    // tamaño del texel ni el bias que hace falta.
    //
    // Una 'bounds' inválida (escena sin mallas, AABB::Empty()) devuelve un
    // encuadre unitario en el origen en vez de propagar los infinitos.
    DirectionalLightFit FitDirectionalLight(
        const glm::vec3& direction,
        const AABB&      bounds,
        f32              padding);

    // Tope de cascadas. El array del bloque std140 tiene exactamente estos slots.
    inline constexpr u32 kMaxCascades = 4u;

    // Reparte [nearPlane, shadowDistance] en 'count' cascadas y escribe el plano
    // LEJANO de cada una en outFar (count valores).
    //
    // lambda mezcla los dos repartos clasicos: 0 = uniforme, 1 = logaritmico. El
    // uniforme deja la cascada 0 inutilmente grande; el logaritmico deja la
    // ultima demasiado gruesa. El default de 0.75 esta cerca del logaritmico
    // porque la resolucion importa mucho mas cerca de la camara.
    void ComputeCascadeSplits(f32 nearPlane, f32 shadowDistance, u32 count,
                              f32 lambda, f32* outFar);

}
}
