add_library(efengine_scene OBJECT)
target_sources(efengine_scene PRIVATE Scene.cpp Camera.cpp)
target_include_directories(efengine_scene PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(efengine_scene PUBLIC efengine_core glm::glm)