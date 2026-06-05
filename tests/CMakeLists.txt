# tests/CMakeLists.txt
# Ejecutable único de tests unitarios. Enlaza contra efengine y doctest.
# Cada TEST_CASE se registra en CTest vía doctest_discover_tests.

add_executable(efengine_tests
    main.cpp
    core/Types.test.cpp
    renderer/VertexLayout.test.cpp
    scene/Camera.test.cpp
)

# Include path raíz: los tests consumen el motor como <efengine/...>.
target_include_directories(efengine_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(efengine_tests PRIVATE efengine doctest::doctest)

# Registra cada TEST_CASE como un test individual de CTest.
doctest_discover_tests(efengine_tests)
