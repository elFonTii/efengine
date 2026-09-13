#pragma once
#include <memory>

namespace efengine {
namespace physics {

    // Dueño del estado global DE PROCESO de Jolt: el allocator por default, la
    // Factory de tipos serializables y el registro de tipos de forma. Nada de
    // eso es por-mundo, asi que no puede vivir adentro de PhysicsWorld.
    //
    // Se construye una vez, se le presta por referencia a cada PhysicsWorld, y
    // tiene que sobrevivirlos a todos. Solo puede haber UNO vivo por proceso: el
    // segundo Create devuelve nullptr en vez de pisar la Factory del primero.
    //
    // Ningun tipo JPH:: se asoma por este header: todo Jolt vive en el .cpp.
    class JoltRuntime {
        public:
            // nullptr si Jolt no inicializa. El cliente puede arrancar sin fisica.
            static std::unique_ptr<JoltRuntime> Create();
            ~JoltRuntime();

            JoltRuntime(const JoltRuntime&)            = delete;
            JoltRuntime& operator=(const JoltRuntime&) = delete;
            JoltRuntime(JoltRuntime&&)                 = delete;
            JoltRuntime& operator=(JoltRuntime&&)      = delete;

        private:
            JoltRuntime();

            struct Impl;
            std::unique_ptr<Impl> m_impl;
    };

}
}
