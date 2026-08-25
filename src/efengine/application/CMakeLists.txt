add_library(efengine_application OBJECT)
# Un archivo de registro por pase en passes/: la carga de shaders de un pase
# vive junto a su registro y no en Application.cpp.
target_sources(efengine_application PRIVATE
    Application.cpp
    DebugUI.cpp
    FramePipeline.cpp
    passes/ShadowPassSetup.cpp
    passes/IblPassSetup.cpp
    passes/DdgiPassSetup.cpp
    passes/AoPassSetup.cpp
    passes/FrameUploadPassSetup.cpp
    passes/IndirectPassSetup.cpp
    passes/SceneTargetPassSetup.cpp
    passes/SkyboxPassSetup.cpp
    passes/ForwardPassSetup.cpp
    passes/DdgiDebugPassSetup.cpp
)
target_include_directories(efengine_application PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(efengine_application PUBLIC
    efengine_core
    efengine_platform
    efengine_renderer
    efengine_resources
    efengine_scene
    imgui
)
