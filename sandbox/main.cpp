#include <efengine/application/Application.h>
#include <efengine/core/Log.h>

int main() {
    EF_LOG_INFO("=== efengine Sandbox: Fase 1 ===");

    efengine::application::Application app;
    app.Run();

    EF_LOG_INFO("=== Sandbox finalizado limpiamente ===");
    return 0;
}
