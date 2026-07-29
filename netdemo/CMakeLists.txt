# netdemo/CMakeLists.txt
# Los dos ejecutables del PoC de red. sandbox/ no se toca.

# SERVIDOR HEADLESS.
# Linkea SOLO efenet (que a su vez trae core, math, binary y enet). Nada de
# efengine completo: si alguien agrega efengine acá, el binario empieza a
# depender de OpenGL y el invariante headless se pierde en silencio.
add_executable(efenet_server server_main.cpp)
target_link_libraries(efenet_server PRIVATE efenet)
target_include_directories(efenet_server PRIVATE ${CMAKE_SOURCE_DIR}/src)
