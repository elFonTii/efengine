# netdemo/CMakeLists.txt
# Los dos ejecutables del PoC de red. sandbox/ no se toca.

# SERVIDOR HEADLESS.
# Linkea SOLO efenet (que a su vez trae core, math, binary y enet). Nada de
# efengine completo: si alguien agrega efengine acá, el binario empieza a
# depender de OpenGL y el invariante headless se pierde en silencio.
add_executable(efenet_server server_main.cpp)
target_link_libraries(efenet_server PRIVATE efenet)
target_include_directories(efenet_server PRIVATE ${CMAKE_SOURCE_DIR}/src)

# CLIENTE. Linkea el motor completo (ventana, render, ImGui) mas efenet.
add_executable(efenet_client
    client_main.cpp
    NetCamera.cpp
    Avatars.cpp
    NetDebugUI.cpp
)
target_link_libraries(efenet_client PRIVATE efengine efenet imgui)
target_include_directories(efenet_client PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Los assets tienen que estar junto al .exe: el cliente carga
# "assets/scenes/sandbox.efe" por ruta relativa.
add_custom_command(TARGET efenet_client POST_BUILD
    COMMAND ${CMAKE_COMMAND}
            -DASSETS_SRC=${CMAKE_SOURCE_DIR}/assets
            -DASSETS_DST=$<TARGET_FILE_DIR:efenet_client>/assets
            -P ${CMAKE_SOURCE_DIR}/cmake/CopyAssets.cmake
    COMMENT "Sincronizando assets junto al ejecutable efenet_client"
    VERBATIM
)
