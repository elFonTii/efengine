#include "efengine/renderer/AoMath.h"

#include <algorithm>

namespace efengine {
namespace renderer {

    glm::vec2 AoProjInfo(const glm::mat4& projection) {
        // Una proyeccion degenerada (matriz identidad en un test) daria 1/0.
        const f32 px = (projection[0][0] != 0.0f) ? 1.0f / projection[0][0] : 0.0f;
        const f32 py = (projection[1][1] != 0.0f) ? 1.0f / projection[1][1] : 0.0f;
        return glm::vec2(px, py);
    }

    f32 AoProjScale(const glm::mat4& projection, u32 height) {
        // 0.5 * alto / tan(fovY/2), y P[1][1] ES 1/tan(fovY/2).
        return 0.5f * static_cast<f32>(height) * projection[1][1];
    }

    AoSettings SanitizeAoSettings(const AoSettings& settings) {
        AoSettings s = settings;
        s.radius          = std::clamp(s.radius,          0.0f,  100.0f);
        s.intensity       = std::clamp(s.intensity,       0.0f,  8.0f);
        s.thickness       = std::clamp(s.thickness,       0.01f, 1.0f);
        s.slices          = std::clamp(s.slices,          1,     8);
        s.steps           = std::clamp(s.steps,           1,     32);
        s.maxScreenRadius = std::clamp(s.maxScreenRadius, 1.0f,  512.0f);
        return s;
    }

    AoPassBlock MakeAoPassBlock(const glm::mat4& view, const glm::mat4& projection,
                                const AoSettings& settings, u32 width, u32 height,
                                i32 blurDirection, i32 scale) {
        // Se sanea aca y no en el caller: este es el ultimo punto antes de que
        // los valores lleguen al shader, donde un cero se vuelve NaN silencioso.
        const AoSettings s = SanitizeAoSettings(settings);

        // Una escala de cero o negativa multiplicaria las coordenadas del texel
        // de guia por cero y todo el target leeria el pixel (0,0) del prepass.
        const i32 esc = (scale > 0) ? scale : 1;

        const glm::vec2 info = AoProjInfo(projection);
        const f32 invW = (width  > 0u) ? 1.0f / static_cast<f32>(width)  : 0.0f;
        const f32 invH = (height > 0u) ? 1.0f / static_cast<f32>(height) : 0.0f;

        AoPassBlock b {};
        b.viewToWorld = glm::inverse(view);
        // 1/resolucion COMPLETA: es lo que convierte el radio de marcha, medido
        // en pixeles de pantalla, a un offset de UV. La UV es la misma para los
        // dos targets, asi que el shader marcha en la grilla fina aunque escriba
        // en la gruesa -- y eso es deseable: los horizontes salen de la
        // profundidad de verdad, no de una submuestreada.
        b.projInfo    = glm::vec4(info.x, info.y, invW, invH);
        b.params0     = glm::vec4(s.radius, s.thickness, s.intensity, s.maxScreenRadius);
        b.params1     = glm::vec4(AoProjScale(projection, height),
                                  static_cast<f32>(esc), 0.0f, 0.0f);
        b.counts      = glm::ivec4(s.slices, s.steps, blurDirection,
                                   static_cast<i32>(s.debugView));
        return b;
    }

    AoBlock MakeAoBlock(const AoContext& ctx, const IndirectContext& indirect) {
        const bool on = (ctx.enabled && ctx.texture != null);

        // Sin el prepass no hay upsample posible: los pesos no tienen contra que
        // comparar. Encenderlo igual lo degradaria a un bilineal a secas, que es
        // exactamente el halo de silueta que el filtro existe para evitar.
        const bool guia = (ctx.depthNormal != null);

        // Los dos flags son INDEPENDIENTES. El AO puede estar a resolucion
        // reducida con la indirecta apagada, y en ese caso pbr.frag igual tiene
        // que subirlo: leerlo con coordenadas de resolucion completa devolveria
        // el cuadrante superior izquierdo estirado sobre toda la pantalla.
        const bool subeIndirecta = guia && indirect.Valid();
        const bool subeAo        = guia && on && ctx.scale > 1;

        // La escala la comparten los dos por construccion (ReducedRes.h). Se
        // toma de quien este activo; si los dos lo estan, tienen que coincidir.
        const i32 escala = subeAo ? ctx.scale : (subeIndirecta ? indirect.scale : 1);

        AoBlock b {};
        b.params = glm::vec4(on ? 1.0f : 0.0f,
                             (on && ctx.bentNormal)  ? 1.0f : 0.0f,
                             (on && ctx.multiBounce) ? 1.0f : 0.0f,
                             static_cast<f32>(ctx.debugView));
        b.upsample = glm::vec4(subeIndirecta ? 1.0f : 0.0f,
                               subeAo        ? 1.0f : 0.0f,
                               static_cast<f32>(escala),
                               0.0f);
        return b;
    }

}
}
