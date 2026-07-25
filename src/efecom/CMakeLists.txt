# src/efecom/CMakeLists.txt
# efecom: Render Hardware Interface (RHI). Librería independiente de efengine.
# glad es PRIVATE: OpenGL es un detalle del backend y no se propaga a nadie.

add_library(efecom STATIC)
target_sources(efecom PRIVATE
    RHIOpenGL.cpp
    Assert.cpp
)

# Raíz de includes pública: se consume como <efecom/...>
target_include_directories(efecom PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(efecom PRIVATE glad)
