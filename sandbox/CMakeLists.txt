# sandbox/CMakeLists.txt
# Define un ejecutable de pruebas para el motor.

add_executable(sandbox main.cpp)

# Linkeamos contra el motor.
target_link_libraries(sandbox PRIVATE efengine)

# Include path (para hacer #include)
target_include_directories(sandbox PRIVATE ${CMAKE_SOURCE_DIR}/src)