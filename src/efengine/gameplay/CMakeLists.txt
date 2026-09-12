# src/efengine/gameplay/CMakeLists.txt
# El puente entre la escena y la fisica: depende de los dos lados porque unirlos
# es su trabajo.
#
# efengine_renderer esta por CollisionMesh.cpp, que lee los vertices de un
# renderer::Model. Es lectura de datos, no del RHI: ningun tipo de renderer se
# asoma por los headers de este modulo.

add_library(efengine_gameplay OBJECT)

target_sources(efengine_gameplay PRIVATE
    CollisionMesh.cpp
    PhysicsBinding.cpp
)

target_include_directories(efengine_gameplay PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(efengine_gameplay PUBLIC
    efengine_core
    efengine_math
    efengine_scene
    efengine_physics
    efengine_renderer
    glm::glm
)
