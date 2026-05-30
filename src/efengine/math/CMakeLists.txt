add_library(efengine_math OBJECT)

target_sources(efengine_math PRIVATE Math.cpp)
target_include_directories(efengine_math PUBLIC ${CMAKE_SOURCE_DIR}/src)

# Math depende de core y de GLM, linkeamos.
target_link_libraries(efengine_math PUBLIC efengine_core glm::glm)