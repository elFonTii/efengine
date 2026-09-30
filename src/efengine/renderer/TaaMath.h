#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/TaaSettings.h>

#include <glm/glm.hpp>

namespace efengine {
namespace renderer {

    struct FrameView;

    // Espejo de taa.frag y de la velocidad de ao/depth_normal.frag. Si se
    // cambia uno, se cambia el otro.

    glm::vec3 RgbToYCoCg(const glm::vec3& c);
    glm::vec3 YCoCgToRgb(const glm::vec3& c);

    // Recorta h hacia el centro de la caja [mn, mx] (Playdead, INSIDE 2016).
    glm::vec3 ClipToAabb(const glm::vec3& h, const glm::vec3& mn, const glm::vec3& mx);

    // Pesos de los 4 texels de un eje, t en [0, 1).
    glm::vec4 CatmullRomWeights(f32 t);

    glm::vec2 ClipToUv(const glm::vec4& clip);

    // Lo que escribe el prepass: uvActual - uvPrevia, ambas sin jitter.
    glm::vec2 VelocityUv(const glm::vec4& clipCur, const glm::vec4& clipPrev);

    // La velocidad que tendria un punto estatico en (uv, depth) de la imagen con
    // jitter. La usa el resolve para el cielo, que no pasa por el prepass.
    glm::vec2 CameraVelocityUv(const glm::vec2& uv, f32 depth,
                               const glm::mat4& invViewProjNoJitter,
                               const glm::mat4& prevViewProjNoJitter,
                               const glm::vec2& jitterUv);

    f32       Luma(const glm::vec3& c);

    // Mezcla con pesos 1/(1+L) (Karis 2014): un brillo aislado no domina.
    glm::vec3 TaaBlend(const glm::vec3& cur, const glm::vec3& hist, f32 alpha);

    TaaBlock  MakeTaaBlock(const FrameView& view, const TaaSettings& settings, bool reset);

}
}
