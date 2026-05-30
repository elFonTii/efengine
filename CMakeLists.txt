# efengine root CMakeLists.txt
# Define todo lo global del proyecto.

# Versión minima de CMake requerida
cmake_minimum_required(VERSION 3.20)

# Nombre del proyecto
project(efengine VERSION 0.1 LANGUAGES CXX)

# Estandar C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Directorio donde CMake guardará archivos de compilación
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Habilitar FetchContent para descargar dependencias
include(FetchContent)

# DEFINIENDO DEPENDENCIAS EXTERNAS
# GLFW - Librería para Windowing y manejo de input
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)

# GLAD - Cargador de funciones OpenGL
# El proyecto CMake de GLAD 2.x vive en la subcarpeta cmake/ del repo,
# por eso SOURCE_SUBDIR: sin esto no se define glad_add_library().
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v2.0.7
    SOURCE_SUBDIR cmake
)

# GLM - Matemáticas para gráficos
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)

# Registrar librerías
FetchContent_MakeAvailable(glfw glad glm)

# GLAD 2.x no crea un target "glad" por sí solo: hay que generarlo con
# glad_add_library(). Aquí pedimos OpenGL 3.3 Core (base compatible y común).
# Para subir de versión, cambia gl:core=3.3 (p. ej. gl:core=4.6).
glad_add_library(glad STATIC API gl:core=3.3)

# SUBDIRECTORIOS
add_subdirectory(src)
add_subdirectory(sandbox)
