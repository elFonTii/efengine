# src/efengine/core/CMakeLists.txt
# Define el módulo core

# Crear librería interna efengine_core
add_library(efengine_core OBJECT)

# Especificar archivos fuente
target_sources(efengine_core PRIVATE
    Types.cpp
    Log.cpp
    Assert.cpp
    Time.cpp
)

# Raíz de includes del proyecto: se incluye como <efengine/core/Log.h>.
# OJO: no exponer la carpeta del módulo (.../core) en el -I, porque en
# Windows (case-insensitive) un header como math/Math.h shadowearía <math.h>.
target_include_directories(efengine_core PUBLIC ${CMAKE_SOURCE_DIR}/src)