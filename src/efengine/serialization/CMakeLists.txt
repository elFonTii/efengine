add_library(efengine_serialization OBJECT)
target_sources(efengine_serialization PRIVATE BinaryWriter.cpp BinaryReader.cpp)
target_include_directories(efengine_serialization PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(efengine_serialization PUBLIC
    efengine_core
    efengine_math
    efengine_renderer
    efengine_resources
    efengine_scene
    glm::glm
)
