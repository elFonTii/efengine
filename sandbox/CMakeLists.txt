# sandbox/CMakeLists.txt
# Define un ejecutable de pruebas para el motor.

add_executable(sandbox main.cpp)

# Linkeamos contra el motor.
target_link_libraries(sandbox PRIVATE efengine)

# Include path (para hacer #include)
target_include_directories(sandbox PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Copia la carpeta assets/ junto al ejecutable tras compilar, para que las
# rutas relativas (p. ej. "assets/madera.jpg") resuelvan sin importar el cwd.
add_custom_command(TARGET sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:sandbox>/assets
    COMMENT "Copiando assets junto al ejecutable sandbox"
)