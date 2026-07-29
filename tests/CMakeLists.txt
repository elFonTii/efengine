# tests/CMakeLists.txt
# Ejecutable único de tests unitarios. Enlaza contra efengine y doctest.
# Cada TEST_CASE se registra en CTest vía doctest_discover_tests.

add_executable(efengine_tests
    main.cpp
    core/Types.test.cpp
    core/Time.test.cpp
    math/Transform.test.cpp
    platform/InputCodes.test.cpp
    platform/Input.test.cpp
    renderer/VertexLayout.test.cpp
    renderer/Vertex.test.cpp
    renderer/PostChain.test.cpp
    renderer/Cubemap.test.cpp
    renderer/ShadowMath.test.cpp
    renderer/Bounds.test.cpp
    renderer/MaterialMap.test.cpp
    renderer/RenderTarget.test.cpp
    renderer/PipelineState.test.cpp
    renderer/ShaderBlocks.test.cpp
    renderer/MaterialBlock.test.cpp
    resources/FileIO.test.cpp
    resources/ResourceManager.test.cpp
    resources/SceneAssets.test.cpp
    scene/NodeHandle.test.cpp
    scene/SceneGraph.test.cpp
    scene/Camera.test.cpp
    scene/CameraController.test.cpp
    scene/Behavior.test.cpp
    scene/SceneGraphClear.test.cpp
    serialization/BinaryIO.test.cpp
    serialization/StringTable.test.cpp
    serialization/EfeFile.test.cpp
    serialization/SceneDocument.test.cpp
    serialization/BehaviorRegistry.test.cpp
    serialization/Truncation.test.cpp
    serialization/SceneSerializer.test.cpp
    efenet/State.test.cpp
    efenet/Simulation.test.cpp
    efenet/Protocol.test.cpp
    efenet/Prediction.test.cpp
    efenet/SnapshotBuffer.test.cpp
    efenet/World.test.cpp
)

# Include path raíz: los tests consumen el motor como <efengine/...>.
target_include_directories(efengine_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(efengine_tests PRIVATE efengine efenet doctest::doctest)

# Registra cada TEST_CASE como un test individual de CTest.
doctest_discover_tests(efengine_tests)
