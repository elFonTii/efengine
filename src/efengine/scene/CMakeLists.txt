add_library(efengine_scene OBJECT)
target_sources(efengine_scene PRIVATE SceneGraph.cpp Camera.cpp CameraController.cpp Behavior.cpp)
target_include_directories(efengine_scene PUBLIC ${CMAKE_SOURCE_DIR}/src)
# efengine_platform: CameraController LLAMA a platform::Input, no solo hereda
# de un header. No hay ciclo: platform depende de core y glfw, nada mas.
target_link_libraries(efengine_scene PUBLIC efengine_core efengine_math efengine_platform efengine_renderer glm::glm)