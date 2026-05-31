add_library(efengine_renderer OBJECT)
target_sources(efengine_renderer PRIVATE Renderer.cpp Context.cpp VertexLayout.cpp Buffer.cpp)
target_include_directories(efengine_renderer PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(efengine_renderer PUBLIC efengine_core glfw glad glm::glm)
