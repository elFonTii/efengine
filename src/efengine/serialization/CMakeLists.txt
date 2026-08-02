# Núcleo binario: lector/escritor de buffers y la cabecera del formato .efe.
# Se separa del resto de serialization porque NO depende de renderer/scene, y
# eso permite que efenet (y por lo tanto el servidor headless) lo consuma sin
# arrastrar OpenGL. Los archivos no se mueven de carpeta: solo cambia el target.
add_library(efengine_binary OBJECT)
target_sources(efengine_binary PRIVATE BinaryWriter.cpp BinaryReader.cpp EfeFile.cpp)
target_include_directories(efengine_binary PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(efengine_binary PUBLIC efengine_core)

add_library(efengine_serialization OBJECT)
target_sources(efengine_serialization PRIVATE
    StringTable.cpp
    SceneDocument.cpp
    BehaviorRegistry.cpp
    MeshGeneratorRegistry.cpp
    SceneSerializer.cpp
)
target_include_directories(efengine_serialization PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(efengine_serialization PUBLIC
    efengine_binary
    efengine_core
    efengine_math
    efengine_renderer
    efengine_resources
    efengine_scene
    glm::glm
)
