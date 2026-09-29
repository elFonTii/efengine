#pragma once
#include <efengine/core/Types.h>
#include <glm/glm.hpp>

namespace efengine {
namespace scene { class Camera; }
namespace renderer {

    // La camara del frame, calculada una vez. Ningun pase vuelve a pedirle
    // matrices a scene::Camera: con jitter, dos fuentes serian dos imagenes.
    struct FrameView {
        glm::mat4 view                 {1.0f};
        glm::mat4 projection           {1.0f};   // con jitter: la que rasteriza
        glm::mat4 projectionNoJitter   {1.0f};
        glm::mat4 viewProjNoJitter     {1.0f};
        glm::mat4 prevViewProjNoJitter {1.0f};
        glm::mat4 invView              {1.0f};
        glm::mat4 invProjection        {1.0f};   // de 'projection'
        glm::vec2 jitterNdc            {0.0f};
        glm::vec2 prevJitterNdc        {0.0f};
        glm::vec3 viewPos              {0.0f};
        u32  width         = 0u;
        u32  height        = 0u;
        u32  frameIndex    = 0u;
        bool jitterEnabled = false;
    };

    struct TemporalSettings {
        bool jitter         = false;
        u32  sequenceLength = 8u;
    };

    // Lo que sobrevive entre frames. valid en false: el proximo frame usa
    // prev == actual (carga de escena, cambio de camara, resize).
    struct FrameHistory {
        glm::mat4 viewProjNoJitter {1.0f};
        glm::vec2 jitterNdc        {0.0f};
        u32  frameIndex = 0u;
        bool valid      = false;
    };

    f32       Halton(u32 index, u32 base);
    // En pixeles, dentro de [-0.5, 0.5]. Muestra (frameIndex % sequenceLength) + 1.
    glm::vec2 HaltonJitterPx(u32 frameIndex, u32 sequenceLength);

    // Avanza history.
    FrameView MakeFrameView(const scene::Camera& camera, u32 width, u32 height,
                            const TemporalSettings& settings, FrameHistory& history);

    // Para las caras de la captura de DDGI: sin jitter y sin historia.
    FrameView MakeStaticFrameView(const glm::mat4& view, const glm::mat4& projection,
                                  const glm::vec3& viewPos, u32 width, u32 height);

}
}
