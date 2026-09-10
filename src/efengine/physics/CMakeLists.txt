# src/efengine/physics/CMakeLists.txt
# Define el modulo physics: la envoltura de Jolt.
#
# Depende SOLO de core, math y glm. No conoce scene, ni renderer, ni el editor.
# Jolt entra por FetchContent desde el CMakeLists raiz.

add_library(efengine_physics OBJECT)

target_sources(efengine_physics PRIVATE
    ShapeDesc.cpp
    JoltRuntime.cpp
    PhysicsWorld.cpp
)

target_include_directories(efengine_physics PUBLIC ${CMAKE_SOURCE_DIR}/src)

# Jolt va PUBLIC a proposito: sus defines de configuracion (JPH_ENABLE_ASSERTS,
# JPH_PROFILE_ENABLED, ...) viajan por la interfaz del target. Si la lib y sus
# consumidores se compilaran con defines distintos, el resultado NO es un error
# de link: es corrupcion de memoria silenciosa. Nunca copiar esos defines a mano.
target_link_libraries(efengine_physics PUBLIC efengine_core efengine_math glm::glm Jolt)
