add_library(efengine_renderer OBJECT)
target_sources(efengine_renderer PRIVATE Renderer.cpp)
target_include_directories(efengine_renderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(efengine_renderer PUBLIC efengine_core glfw glad glm::glm)
