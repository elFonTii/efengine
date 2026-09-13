#include <efengine/physics/JoltRuntime.h>

#include <efengine/core/Log.h>

// Jolt.h SIEMPRE va primero: define los macros de configuracion que el resto de
// sus headers necesitan.
#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>

#include <cstdarg>
#include <cstdio>

namespace efengine {
namespace physics {

namespace {
    // Jolt escribe sus diagnosticos por un puntero a funcion global. Sin esto
    // van a stdout crudo y se pierden entre el resto del log del motor.
    void traceAlLog(const char* fmt, ...) {
        char buffer[1024];
        va_list args;
        va_start(args, fmt);
        std::vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        EF_LOG_INFO("[Jolt] %s", buffer);
    }

#ifdef JPH_ENABLE_ASSERTS
    bool assertAlLog(const char* expresion, const char* mensaje,
                     const char* archivo, JPH::uint linea) {
        EF_LOG_ERROR("[Jolt] assert en %s:%u -- %s %s",
                     archivo, static_cast<unsigned>(linea), expresion,
                     mensaje != nullptr ? mensaje : "");
        return true;   // true = frenar en el debugger
    }
#endif
}

struct JoltRuntime::Impl {
    // Jolt pide literalmente 'Factory::sInstance = new Factory'. El README
    // prohibe new crudo, asi que el dueño es este unique_ptr y a sInstance se
    // le presta el .get(): sigue siendo RAII y el destructor no se puede olvidar.
    std::unique_ptr<JPH::Factory> factory;
};

JoltRuntime::JoltRuntime() : m_impl(std::make_unique<Impl>()) {}

std::unique_ptr<JoltRuntime> JoltRuntime::Create() {
    if (JPH::Factory::sInstance != nullptr) {
        EF_LOG_ERROR("JoltRuntime::Create: ya hay un runtime vivo; Jolt admite uno solo por proceso");
        return nullptr;
    }

    // El allocator va primero: la Factory ya se aloca con el.
    JPH::RegisterDefaultAllocator();

    JPH::Trace = traceAlLog;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = assertAlLog;)

    // El ctor es privado: make_unique no lo alcanza (mismo caso que DdgiPass).
    std::unique_ptr<JoltRuntime> runtime(new JoltRuntime());

    runtime->m_impl->factory = std::make_unique<JPH::Factory>();
    JPH::Factory::sInstance  = runtime->m_impl->factory.get();

    JPH::RegisterTypes();

    EF_LOG_INFO("JoltRuntime: Jolt inicializado");
    return runtime;
}

JoltRuntime::~JoltRuntime() {
    // Orden obligatorio: los tipos se desregistran mientras la Factory todavia
    // existe, y recien despues se suelta. Al reves, Jolt lee memoria liberada.
    JPH::UnregisterTypes();
    JPH::Factory::sInstance = nullptr;
    m_impl->factory.reset();
}

}
}
