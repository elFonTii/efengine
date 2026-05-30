add_library(efengine_platform OBJECT)
target_sources(efengine_platform PRIVATE Window.cpp)
target_include_directories(efengine_platform PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(efengine_platform PUBLIC efengine_core glfw glad)