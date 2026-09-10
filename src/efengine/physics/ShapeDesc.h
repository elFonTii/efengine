#pragma once
#include <efengine/core/Types.h>

#include <glm/glm.hpp>

namespace efengine {
namespace physics {

    // Duplicados A PROPOSITO de scene::ShapeKind y scene::MotionType. physics no
    // conoce scene: son diez lineas y son exactamente lo que evita esa arista
    // hacia arriba. El switch que traduce uno al otro vive en el binding del
    // ciclo 3b, que es el unico que ve los dos lados.
    enum class ShapeKind  { Box, Sphere, Capsule, Mesh };
    enum class MotionType { Static, Kinematic, Dynamic };

    struct ShapeDesc {
        ShapeKind kind = ShapeKind::Box;

        // Box: halfExtents - Sphere: (r, ., .) - Capsule: (r, halfHeight, .)
        glm::vec3 params { 0.5f };

        // Solo si kind == Mesh. Punteros NO duenos: solo tienen que seguir vivos
        // durante la llamada a PhysicsWorld::CreateBody, que copia la geometria
        // adentro de la forma. Guardarlos para despues es un dangling pointer.
        const f32* meshPositions   = nullptr;   // xyz consecutivos
        u32        meshVertexCount = 0;
        const u32* meshIndices     = nullptr;
        u32        meshIndexCount  = 0;
    };

    // Resultado de aplicar la escala del transform de mundo a una descripcion.
    //
    // Las formas de Jolt no escalan gratis, y ademas SphereShape exige los tres
    // ejes iguales y CapsuleShape exige X == Z. Una caja escalada 2x1x3 es
    // valida; una esfera escalada 2x1x3 no existe.
    struct ScaledShape {
        ShapeDesc desc;                     // params ya escalados (primitivas)
        glm::vec3 residualScale { 1.0f };   // para el ScaledShape de Jolt (solo Mesh)
        bool      approximated = false;     // hubo que colapsar ejes: el caller loguea
        bool      degenerate   = false;     // algo quedo en ~0: la forma no existe
    };

    // Aritmetica pura: no toca Jolt. Se aproxima en vez de rechazar porque el
    // nodo tiene que tener cuerpo igual: un collider un poco mas gordo se ve;
    // caerse por el piso, no. Es lo que hacen Unity y Unreal.
    ScaledShape ScaleShape(const ShapeDesc& desc, const glm::vec3& scale);

}
}
