# src/efengine/CMakeLists.txt
# Define la librería efengine como target.

# Agregar subdirectorios de módulos
add_subdirectory(core)
add_subdirectory(math)
add_subdirectory(platform)
add_subdirectory(renderer)
add_subdirectory(resources)
add_subdirectory(scene)
add_subdirectory(application)

# Crear librería estática efengine
# que combine todos los módulos
add_library(efengine STATIC)

# Raíz de includes pública: se consume como <efengine/...>
target_include_directories(efengine PUBLIC ${CMAKE_SOURCE_DIR}/src)

# Enlazar módulos
target_link_libraries(efengine PUBLIC
    efengine_core
    efengine_math
    efengine_platform
    efengine_renderer
    efengine_resources
    efengine_scene
    efengine_application
)