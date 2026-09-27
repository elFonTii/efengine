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

    // Encuadre de UNA cascada. A diferencia de DirectionalLightFit, guarda el
    // centro y el radio porque el culling y el panel los necesitan despues.
    struct CascadeFit {
        glm::mat4 matrix         { 1.0f };
        glm::vec3 center         { 0.0f };  // centro de la esfera, en mundo (ya cuantizado)
        f32       radius         { 1.0f };  // radio de la esfera de la rebanada
        f32       texelWorldSize { 1.0f };  // cuanto mide un texel de ESTA cascada, en metros
        f32       depthRange     { 1.0f };  // far - near de la caja, en metros
        f32       splitFar       { 1.0f };  // el corte lejano, en distancia de vista
    };

    // Encuadra la rebanada [sliceNear, sliceFar] del frustum de la camara.
    //
    // Va por la esfera que contiene la rebanada y no por su AABB por dos razones.
    // La primera ya vale para FitDirectionalLight: la esfera es invariante a la
    // direccion de la luz. La segunda es propia de las cascadas y es la que
    // importa: el radio de esa esfera depende solo de sliceNear/sliceFar y del
    // fov, NO de hacia donde mira la camara. O sea que el texel mide siempre lo
    // mismo. Con la AABB del frustum, girar la cabeza cambiaria el tamano del
    // texel y las sombras respirarian.
    //
    // 'lightExtension' corre el plano cercano hacia la luz: un objeto FUERA de la
    // rebanada pero mas arriba en la direccion del sol igual proyecta sombra
    // adentro. Solo toca la profundidad, nunca el encuadre lateral.
    CascadeFit FitCascade(const glm::vec3& direction, const glm::mat4& invView,
                          f32 fovDeg, f32 aspect, f32 sliceNear, f32 sliceFar,
                          u32 resolution, f32 lightExtension);

    // AABB de mundo que hay que dibujar en el shadow map de esta cascada.
    //
    // NO es la caja de la cascada: es la caja estirada hacia DONDE ESTA LA LUZ,
    // por lo mismo que FitCascade corre el plano cercano. La AABB es un superset
    // conservador de la caja ortografica (que en general esta rotada), y eso esta
    // bien: de mas dibuja una submalla que no se ve, de menos borra una sombra.
    AABB CascadeCullVolume(const CascadeFit& fit, const glm::vec3& direction);

}
}
