#include "efengine/renderer/FrameView.h"

#include <efengine/scene/Camera.h>

namespace efengine {
namespace renderer {

    f32 Halton(u32 index, u32 base) {
        f32 f = 1.0f;
        f32 r = 0.0f;
        while (index > 0u) {
            f /= static_cast<f32>(base);
            r += f * static_cast<f32>(index % base);
            index /= base;
        }
        return r;
    }

    glm::vec2 HaltonJitterPx(u32 frameIndex, u32 sequenceLength) {
        const u32 n = sequenceLength == 0u ? 1u : sequenceLength;
        const u32 i = (frameIndex % n) + 1u;
        return glm::vec2(Halton(i, 2u) - 0.5f, Halton(i, 3u) - 0.5f);
    }

    FrameView MakeFrameView(const scene::Camera& camera, u32 width, u32 height,
                            const TemporalSettings& settings, FrameHistory& history) {
        FrameView v;
        v.view               = camera.ViewMatrix();
        v.projectionNoJitter = camera.ProjectionMatrix();
        v.projection         = v.projectionNoJitter;
        v.viewPos            = camera.Position();
        v.width              = width;
        v.height             = height;
        v.frameIndex         = history.frameIndex;

        if (settings.jitter && width > 0u && height > 0u) {
            const glm::vec2 px = HaltonJitterPx(history.frameIndex, settings.sequenceLength);
            v.jitterNdc = glm::vec2(px.x * 2.0f / static_cast<f32>(width),
                                    px.y * 2.0f / static_cast<f32>(height));
            // En una perspectiva w = -z_vista: restar en [2][x] suma en NDC.
            v.projection[2][0] -= v.jitterNdc.x;
            v.projection[2][1] -= v.jitterNdc.y;
            v.jitterEnabled = true;
        }

        v.viewProjNoJitter = v.projectionNoJitter * v.view;
        v.invView          = glm::inverse(v.view);
        v.invProjection    = glm::inverse(v.projection);

        if (history.valid) {
            v.prevViewProjNoJitter = history.viewProjNoJitter;
            v.prevJitterNdc        = history.jitterNdc;
        } else {
            v.prevViewProjNoJitter = v.viewProjNoJitter;
            v.prevJitterNdc        = v.jitterNdc;
        }

        history.viewProjNoJitter = v.viewProjNoJitter;
        history.jitterNdc        = v.jitterNdc;
        history.valid            = true;
        ++history.frameIndex;
        return v;
    }

    FrameView MakeStaticFrameView(const glm::mat4& view, const glm::mat4& projection,
                                  const glm::vec3& viewPos, u32 width, u32 height) {
        FrameView v;
        v.view                 = view;
        v.projection           = projection;
        v.projectionNoJitter   = projection;
        v.viewProjNoJitter     = projection * view;
        v.prevViewProjNoJitter = v.viewProjNoJitter;
        v.invView              = glm::inverse(view);
        v.invProjection        = glm::inverse(projection);
        v.viewPos              = viewPos;
        v.width                = width;
        v.height               = height;
        return v;
    }

}
}
