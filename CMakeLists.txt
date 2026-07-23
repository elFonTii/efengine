# efengine root CMakeLists.txt
# Define todo lo global del proyecto.

# Versión minima de CMake requerida
cmake_minimum_required(VERSION 3.20)

# Nombre del proyecto
project(efengine VERSION 0.1 LANGUAGES CXX C)

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

# doctest - Framework de tests unitarios (header-only).
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.12
)

# stb - Carga/decodificación de imágenes (header-only). stb no usa tags de
# versión; se fija a un commit concreto del master para reproducibilidad.
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG f0569113c93ad095470c54bf34a17b36646bbbb5
)

# assimp - Importador de modelos 3D (FBX, OBJ, glTF, ...). En v1 solo usamos el
# importador FBX. Build mínimo: estático, sin tests/tools/exportadores.
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.4.3
)

# Dear ImGui - GUI inmediata para tooling de debug en runtime. No trae
# CMakeLists propio: tras poblarlo se arma un target manual (igual que stb).
# Se fija a un tag estable (no docking branch: un solo panel no lo necesita).
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.5
)

# Opciones de GLFW: no compilar sus docs/tests/examples.
# GLFW crea un target 'docs' propio si encuentra Doxygen, que colisiona con el
# nuestro. Apagarlos evita la colisión y acelera el build.
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# doctest: no compilar sus propios tests ni su main en lib estática.
set(DOCTEST_WITH_TESTS              OFF CACHE BOOL "" FORCE)
set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF CACHE BOOL "" FORCE)

# Opción para que un consumidor que embeba efengine pueda apagar los tests.
option(EFENGINE_BUILD_TESTS "Compilar los tests unitarios de efengine" ON)

# Registrar librerías
FetchContent_MakeAvailable(glfw glad glm stb)
if(EFENGINE_BUILD_TESTS)
    FetchContent_MakeAvailable(doctest)
endif()

# Opciones de assimp: build mínimo. Deben fijarse ANTES de MakeAvailable(assimp).
# Solo el importador FBX; sin exportadores, tests, tools ni samples.
# BUILD_SHARED_LIBS OFF => assimp estático: no hay DLL que copiar junto al .exe.
# (glfw/glad/glm/stb ya se poblaron arriba, así que esto solo afecta a assimp.)
set(ASSIMP_BUILD_TESTS                    OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS             OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL                        OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT                      ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER             ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB                     ON  CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS             OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS                     OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(assimp)

# stb no trae CMakeLists: tras poblarlo, exponemos su carpeta como include
# mediante un target INTERFACE. La implementación se compila una sola vez en
# src/efengine/renderer/stb_image_impl.cpp.
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

# Dear ImGui: no trae CMakeLists. Lo poblamos y compilamos un target STATIC con
# el core + los backends GLFW/OpenGL3. El backend GL3 usa su loader interno, así
# que sólo necesita glfw para el backend de ventana/input.
FetchContent_MakeAvailable(imgui)
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw)

# GLAD 2.x no crea un target "glad" por sí solo: hay que generarlo con
# glad_add_library().
glad_add_library(glad STATIC API gl:core=4.5)

# DOCUMENTACIÓN (Doxygen)
# Target opcional 'docs'. Si Doxygen no está instalado, simplemente se omite
# y el build normal no se ve afectado.
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_INPUT_DIR  ${CMAKE_SOURCE_DIR})
    set(DOXYGEN_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/docs)
    set(DOXYFILE_IN  ${CMAKE_SOURCE_DIR}/cmake/Doxyfile.in)
    set(DOXYFILE_OUT ${CMAKE_BINARY_DIR}/Doxyfile)

    # Rellena las variables @...@ de la plantilla con los valores de CMake.
    configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generando documentación con Doxygen en ${DOXYGEN_OUTPUT_DIR}/html"
        VERBATIM
    )
else()
    message(STATUS "Doxygen no encontrado: el target 'docs' no estará disponible.")
endif()

# SUBDIRECTORIOS
add_subdirectory(src)
add_subdirectory(sandbox)

# TESTS (doctest + CTest)
if(EFENGINE_BUILD_TESTS)
    enable_testing()
    # El helper doctest_discover_tests vive en el repo de doctest.
    include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
    add_subdirectory(tests)
endif()
