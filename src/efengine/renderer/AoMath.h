#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/AoSettings.h>
#include <efengine/renderer/AoContext.h>
#include <efengine/renderer/IndirectContext.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <glm/glm.hpp>

namespace efengine {
namespace renderer {

    // Los dos factores que reconstruyen una posicion view-space desde una UV y
    // una profundidad lineal: xy = (1/P[0][0], 1/P[1][1]).
    //
    // Sale de la matriz y no de fov/aspect a proposito: asi funciona con
    // cualquier proyeccion, y el test es de dos lineas.
    glm::vec2 AoProjInfo(const glm::mat4& projection);

    // Pixeles por metro a 1 metro de distancia: 0.5 * alto / tan(fovY/2).
    // Es lo que convierte el radio en METROS del panel al radio en PIXELES con
    // el que el shader marcha.
    f32 AoProjScale(const glm::mat4& projection, u32 height);

    // Clampea todo a rangos que el shader pueda usar sin producir NaN. Los
    // valores vienen de sliders de ImGui: son recuperables, se corrigen y se
    // sigue, no se assertea (la misma politica que SanitizeGrid).
    //
    // radius = 0 es LEGAL y se conserva: es el control nulo de la tabla de
    // verificacion (radio 0 debe dar visibilidad 1 en todas partes).
    AoSettings SanitizeAoSettings(const AoSettings& settings);

    // width/height son la resolucion COMPLETA de la pantalla, no la del target.
    // Los dos numeros que salen de ahi -- projScale (pixeles por metro) y
    // projInfo.zw (1/resolucion, que convierte offsets en pixeles a UV) --
    // describen la camara, no el buffer: calcularlos sobre un target a media
    // resolucion duplicaria el radio de marcha en metros sin que nadie toque el
    // slider.
    //
    // scale es cuantos texels de resolucion completa cubre uno del target por
    // eje (1 = full, 2 = media). Viaja al shader para que sepa en que texel del
    // prepass -- que SIEMPRE esta a resolucion completa -- leer su guia.
    //
    // blurDirection: 0 = horizontal, 1 = vertical. gtao.frag lo ignora.
    AoPassBlock MakeAoPassBlock(const glm::mat4& view, const glm::mat4& projection,
                                const AoSettings& settings, u32 width, u32 height,
                                i32 blurDirection, i32 scale = 1);

    // Apaga params.x si no hay textura, aunque settings.enabled sea true: es el
    // caso "no hay AoPass" (fallo de shader), donde pbr.frag tiene que caer al
    // ao del material en vez de samplear una unidad sin contenido.
    //
    // El segundo contexto llena `upsample`, que es como pbr.frag reconstruye lo
    // que se resolvio a resolucion reducida. Los dos van juntos porque comparten
    // el mismo binding y la misma decision: si el prepass-guia no esta, ni el
    // upsample de la indirecta ni el del AO son posibles, y los dos tienen que
    // apagarse a la vez o el shader lee una guia que no existe.
    AoBlock MakeAoBlock(const AoContext& ctx, const IndirectContext& indirect);

}
}
