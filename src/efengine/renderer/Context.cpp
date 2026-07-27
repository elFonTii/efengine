#include "Context.h"

#include <efecom/RHI.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>       // glfwGetProcAddress

#include <efengine/platform/Window.h>
#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>

namespace efengine {
namespace renderer {

    Context::Context(platform::Window& window) {
        const bool ok = efecom::Initialize(glfwGetProcAddress);
        EF_ASSERT(ok, "efecom::Initialize failed: could not load graphics API functions");

        efecom::SetViewport(0, 0, window.GetWidth(), window.GetHeight()); // Si lo inicializo en Window el contexto de opengl no está activo todavía "access violation -1073741819"
        efecom::SetPresentExtent(window.GetWidth(), window.GetHeight());
        log_gpu_info();
    }

    void Context::log_gpu_info() const {
        const efecom::DeviceInfo info = efecom::GetDeviceInfo();
        EF_LOG_INFO("Graphics API   : %s", info.apiVersion);
        EF_LOG_INFO("GPU Renderer   : %s", info.renderer);
        EF_LOG_INFO("GPU Vendor     : %s", info.vendor);
        EF_LOG_INFO("Shading Lang   : %s", info.shadingLanguageVersion);
    }

} // namespace renderer
} // namespace efengine
