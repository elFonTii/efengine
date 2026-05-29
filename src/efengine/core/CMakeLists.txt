# src/efengine/core/CMakeLists.txt
# Define el módulo core

# Crear librería interna efengine_core
add_library(efengine_core OBJECT)

# Especificar archivos fuente
target_sources(efengine_core PRIVATE
    Types.cpp
    Log.cpp
    Assert.cpp
)

# Headers públicos del módulo
target_include_directories(efengine_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})