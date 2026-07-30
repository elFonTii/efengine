# sandbox/CMakeLists.txt
# Define un ejecutable de pruebas para el motor.

add_executable(sandbox
    main.cpp
    EditorUI.cpp
    AssetCatalog.cpp
    AuthoringUI.cpp
    TestScene.cpp
    SunGizmo.cpp
)

# Linkeamos contra el motor.
target_link_libraries(sandbox PRIVATE efengine)

# Include path (para hacer #include)
target_include_directories(sandbox PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Sincroniza la carpeta assets/ junto al ejecutable tras compilar, para que las
# rutas relativas (p. ej. "assets/madera.jpg") resuelvan sin importar el cwd.
# CopyAssets.cmake copia sólo los archivos nuevos o modificados (ver el comentario
# del script); un copy_directory a secas reescribía los 145 MB en cada build.
add_custom_command(TARGET sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND}
            -DASSETS_SRC=${CMAKE_SOURCE_DIR}/assets
            -DASSETS_DST=$<TARGET_FILE_DIR:sandbox>/assets
            -P ${CMAKE_SOURCE_DIR}/cmake/CopyAssets.cmake
    COMMENT "Sincronizando assets junto al ejecutable sandbox"
    VERBATIM
)