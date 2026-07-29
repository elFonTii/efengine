# efenet: capa de red multijugador.
#
# INVARIANTE: esta lista de dependencias NO incluye efengine_renderer,
# efengine_scene, efengine_platform ni efengine_application. Es lo que garantiza
# que efenet_server sea headless de verdad: si alguien incluye un header de esos
# módulos acá adentro, no compila.
add_library(efenet STATIC)

target_sources(efenet PRIVATE Version.cpp Simulation.cpp)

target_include_directories(efenet PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(efenet PUBLIC
    efengine_core
    efengine_math
    efengine_binary
    glm::glm
    enet
)

# ENet en Windows necesita los sockets de Winsock y el timer de multimedia.
# Su CMakeLists no siempre los propaga a los consumidores.
if(WIN32)
    target_link_libraries(efenet PUBLIC ws2_32 winmm)
endif()
