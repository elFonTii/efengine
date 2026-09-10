#include <doctest/doctest.h>
#include <efengine/physics/JoltRuntime.h>

using efengine::physics::JoltRuntime;

TEST_CASE("JoltRuntime: se crea y arranca vivo") {
    const std::unique_ptr<JoltRuntime> rt = JoltRuntime::Create();
    REQUIRE(rt != nullptr);
}

TEST_CASE("JoltRuntime: se puede destruir y volver a crear en el mismo proceso") {
    // El estado de Jolt es GLOBAL de proceso (allocator, Factory, tipos
    // registrados). Si el destructor deja algo sin desarmar, el segundo Create
    // devuelve nullptr o revienta. Este test es la red que atrapa eso, y ademas
    // es lo que permite que cada TEST_CASE de abajo arme su propio runtime.
    {
        const std::unique_ptr<JoltRuntime> primero = JoltRuntime::Create();
        REQUIRE(primero != nullptr);
    }
    {
        const std::unique_ptr<JoltRuntime> segundo = JoltRuntime::Create();
        REQUIRE(segundo != nullptr);
    }
}

TEST_CASE("JoltRuntime: dos runtimes vivos a la vez se rechaza") {
    // Jolt tiene una sola Factory por proceso: dos runtimes simultaneos se
    // pisarian. Falla ruidoso con nullptr, no con un crash a los diez minutos.
    const std::unique_ptr<JoltRuntime> primero = JoltRuntime::Create();
    REQUIRE(primero != nullptr);

    const std::unique_ptr<JoltRuntime> segundo = JoltRuntime::Create();
    CHECK(segundo == nullptr);
}
