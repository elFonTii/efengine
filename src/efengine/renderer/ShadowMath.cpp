#include "efengine/renderer/ShadowMath.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace efengine {
namespace renderer {

    glm::mat4 ComputeDirectionalLightMatrix(
        const glm::vec3& direction, const glm::vec3& center,
        f32 orthoHalfSize, f32 distance, f32 nearPlane, f32 farPlane) {

        const glm::vec3 dir = glm::normalize(direction);
        const glm::vec3 eye = center - dir * distance;

        const glm::vec3 up = (std::abs(dir.y) > 0.999f)
                           ? glm::vec3(0.0f, 0.0f, 1.0f)
                           : glm::vec3(0.0f, 1.0f, 0.0f);

        const glm::mat4 view = glm::lookAt(eye, center, up);
        const glm::mat4 proj = glm::ortho(-orthoHalfSize, orthoHalfSize,
                                          -orthoHalfSize, orthoHalfSize,
                                          nearPlane, farPlane);
        return proj * view;
    }

    DirectionalLightFit FitDirectionalLight(
        const glm::vec3& direction, const AABB& bounds, f32 padding) {

        // Sin mallas no hay nada que encuadrar. Un metro en el origen mantiene
        // la matriz finita; el pase igual no va a dibujar nada.
        const bool  valida = bounds.Valid();
        const glm::vec3 center = valida ? bounds.Center() : glm::vec3(0.0f);
        // El piso de EPSILON cubre la escena de un solo punto (radio 0), que
        // haria una caja ortografica degenerada.
        const f32   radius = valida ? glm::max(bounds.Radius(), EPSILON) : 1.0f;

        const f32 margen = glm::max(padding, 0.0f);
        const f32 half   = radius + margen;

        // El ojo se para justo sobre la esfera de encuadre: la escena queda
        // entre las profundidades [margen, 2*radius + margen], con 'margen' de
        // aire contra el near y contra el far. De ahi que far == 2*half.
        DirectionalLightFit fit;
        fit.orthoHalfSize = half;
        fit.depthRange    = 2.0f * half;
        fit.matrix        = ComputeDirectionalLightMatrix(
            direction, center, half, half, 0.0f, fit.depthRange);
        return fit;
    }

    void ComputeCascadeSplits(f32 nearPlane, f32 shadowDistance, u32 count,
                              f32 lambda, f32* outFar) {
        if (outFar == null || count == 0u) return;
        if (count > kMaxCascades) count = kMaxCascades;

        // El reparto logaritmico divide por near, asi que un near de cero o un
        // rango invertido lo mandan a infinito. El piso lo mantiene finito; el
        // encuadre de una cascada degenerada igual no dibuja nada.
        const f32 cerca  = glm::max(nearPlane, EPSILON);
        const f32 lejos  = glm::max(shadowDistance, cerca + EPSILON);
        const f32 mezcla = glm::clamp(lambda, 0.0f, 1.0f);

        for (u32 i = 0; i < count; ++i) {
            const f32 fraccion    = static_cast<f32>(i + 1u) / static_cast<f32>(count);
            const f32 uniforme    = cerca + (lejos - cerca) * fraccion;
            const f32 logaritmico = cerca * std::pow(lejos / cerca, fraccion);
            outFar[i] = glm::mix(uniforme, logaritmico, mezcla);
        }
        // El ultimo corte tiene que dar EXACTAMENTE shadowDistance: si queda
        // corto por error de redondeo, se abre una franja sin sombra justo en el
        // borde, que es donde menos se la espera.
        outFar[count - 1u] = lejos;
    }


    CascadeFit FitCascade(const glm::vec3& direction, const glm::mat4& invView,
                          f32 fovDeg, f32 aspect, f32 sliceNear, f32 sliceFar,
                          u32 resolution, f32 lightExtension) {

        const f32 cerca = glm::max(sliceNear, EPSILON);
        const f32 lejos = glm::max(sliceFar, cerca + EPSILON);
        const f32 res   = static_cast<f32>(resolution > 0u ? resolution : 1u);

        // Las 8 esquinas de la rebanada en espacio de VISTA. La camara mira por
        // -Z, asi que las profundidades van negadas.
        const f32 mitadV = std::tan(glm::radians(fovDeg * 0.5f));
        const f32 mitadH = mitadV * aspect;

        glm::vec3 esquinas[8];
        for (int i = 0; i < 8; ++i) {
            const f32 z = (i & 4) ? lejos : cerca;
            esquinas[i] = glm::vec3(((i & 1) ? 1.0f : -1.0f) * mitadH * z,
                                    ((i & 2) ? 1.0f : -1.0f) * mitadV * z,
                                    -z);
        }

        // Centro y radio EN ESPACIO DE VISTA: aca es donde la invariancia se
        // gana. El centro cae sobre el eje -Z y el radio sale de la geometria de
        // la rebanada, sin que la orientacion de la camara entre en la cuenta.
        glm::vec3 centroVista(0.0f);
        for (const glm::vec3& e : esquinas) centroVista += e;
        centroVista /= 8.0f;

        f32 radio = 0.0f;
        for (const glm::vec3& e : esquinas) {
            radio = glm::max(radio, glm::length(e - centroVista));
        }
        // Redondear hacia arriba mata el jitter de ultimo bit del radio, que si
        // no cambia el tamano del texel de un frame al otro y reintroduce el
        // hervor que la cuantizacion del centro viene a sacar.
        radio = std::ceil(radio * 16.0f) / 16.0f;
        radio = glm::max(radio, EPSILON);

        const f32 texel = 2.0f * radio / res;

        glm::vec3 centro = glm::vec3(invView * glm::vec4(centroVista, 1.0f));

        // Cuantizar el centro a multiplos enteros del texel, medido en el marco
        // de la LUZ: es en ese marco donde la grilla del shadow map vive. Sin
        // esto el centro se corre una fraccion de texel por frame y los bordes
        // de sombra hierven al caminar.
        const glm::vec3 dir = glm::normalize(direction);
        const glm::vec3 up  = (std::abs(dir.y) > 0.999f) ? glm::vec3(0.0f, 0.0f, 1.0f)
                                                         : glm::vec3(0.0f, 1.0f, 0.0f);
        // El marco se ancla al ORIGEN DEL MUNDO, no al centro: si el ojo de la
        // luz se pusiera a partir de 'centro', el centro caeria siempre en el
        // mismo punto del marco y el floor no cuantizaria nada.
        const glm::mat4 luzView    = glm::lookAt(glm::vec3(0.0f), dir, up);
        const glm::mat4 luzInversa = glm::inverse(luzView);

        glm::vec3 enLuz = glm::vec3(luzView * glm::vec4(centro, 1.0f));
        // Tambien la profundidad: la caja se arma RELATIVA al centro, asi que
        // correrlo a lo largo de la luz no cambia que queda adentro, y dejarla
        // libre filtraba el jitter de la camara de vuelta al centro de mundo.
        enLuz.x = std::floor(enLuz.x / texel) * texel;
        enLuz.y = std::floor(enLuz.y / texel) * texel;
        enLuz.z = std::floor(enLuz.z / texel) * texel;
        centro  = glm::vec3(luzInversa * glm::vec4(enLuz, 1.0f));

        const f32 extension = glm::max(lightExtension, 0.0f);

        CascadeFit fit;
        fit.center         = centro;
        fit.radius         = radio;
        fit.texelWorldSize = texel;
        fit.depthRange     = 2.0f * radio + extension;
        fit.splitFar       = lejos;
        // El ojo retrocede la extension ADEMAS del radio: asi el near sigue
        // valiendo 0 sobre la esfera y lo unico que crece es lo que entra por
        // detras. El half lateral queda en 'radio', intacto.
        fit.matrix = ComputeDirectionalLightMatrix(direction, centro, radio,
                                                   radio + extension,
                                                   0.0f, fit.depthRange);
        return fit;
    }


    AABB CascadeCullVolume(const CascadeFit& fit, const glm::vec3& direction) {
        const glm::vec3 dir = glm::normalize(direction);
        // depthRange = 2*radio + extension, asi que esto recupera la extension.
        const f32 extension = glm::max(fit.depthRange - 2.0f * fit.radius, 0.0f);

        AABB v;
        v.min = fit.center - glm::vec3(fit.radius);
        v.max = fit.center + glm::vec3(fit.radius);

        // -dir apunta a donde ESTA la luz. Estirar por ahi y solo por ahi:
        // estirar en los dos sentidos duplicaria la geometria dibujada sin
        // agregar una sola sombra.
        const glm::vec3 haciaLaLuz = -dir * extension;
        v.min += glm::min(haciaLaLuz, glm::vec3(0.0f));
        v.max += glm::max(haciaLaLuz, glm::vec3(0.0f));
        return v;
    }

}
}
