add_library(efengine_application OBJECT)
target_sources(efengine_application PRIVATE Application.cpp)
target_include_directories(efengine_application PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(efengine_application PUBLIC
    efengine_core
    efengine_platform
    efengine_renderer
    efengine_resources
    efengine_scene
)
