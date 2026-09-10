# src/efengine/physics/CMakeLists.txt
# Define el modulo physics: la envoltura de Jolt.
#
# Depende SOLO de core, math y glm. No conoce scene, ni renderer, ni el editor.
# El target Jolt se agrega en el ciclo siguiente de este mismo plan.

add_library(efengine_physics OBJECT)

target_sources(efengine_physics PRIVATE
    ShapeDesc.cpp
)

target_include_directories(efengine_physics PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(efengine_physics PUBLIC efengine_core efengine_math glm::glm)
