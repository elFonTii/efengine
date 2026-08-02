#include <efenet/Server.h>
#include <efenet/Types.h>

#include <efengine/core/Time.h>
#include <efengine/core/Log.h>

#include <chrono>
#include <thread>

// Servidor headless de efenet.
//
// NO incluye nada de efengine/renderer, /scene, /platform ni /application: eso
// es lo que lo mantiene sin dependencia de OpenGL, y se verifica con
// `dumpbin /dependents` al final de esta tarea.
int main() {
    using namespace efengine;

    EF_LOG_INFO("=== efenet: servidor ===");

    efenet::Server server(efenet::kPort);
    if (!server.Ok()) {
        EF_LOG_ERROR("efenet: no se pudo abrir el puerto %u", static_cast<u32>(efenet::kPort));
        return 1;
    }

    core::Time clock;
    f32 acumulador = 0.0f;

    // core::Time ya clampea el delta a 100 ms (SetMaxDelta), asi que un stall
    // del SO produce como mucho 6 ticks de catch-up en vez de una espiral de la
    // muerte. No hace falta guarda extra: el motor ya la tiene.
    while (true) {
        clock.Tick();
        acumulador += clock.DeltaTime();

        while (acumulador >= efenet::kTickDt) {
            server.Tick();
            acumulador -= efenet::kTickDt;
        }

        // Sin esto el loop quema un core entero girando en vacio.
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}
