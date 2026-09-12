# sandbox/CMakeLists.txt
# Define un ejecutable de pruebas para el motor.

# Un archivo de panel por pase en panels/: EditorUI.cpp no nombra ningun pase.
add_executable(sandbox
    main.cpp
    EditorUI.cpp
    AssetCatalog.cpp
    AuthoringUI.cpp
    TestScene.cpp
    SunGizmo.cpp
    ColliderGizmo.cpp
    panels/Panels.cpp
    panels/ShadowPanel.cpp
    panels/DdgiPanel.cpp
    panels/AoPanel.cpp
)

# Linkeamos contra el motor.
target_link_libraries(sandbox PRIVATE efengine)

# Include path (para hacer #include). El propio sandbox/ tambien: asi los
# archivos de panels/ pueden incluir "panels/PassPanel.h" y "EditorUI.h" con la
# misma ruta que usa EditorUI.cpp, sin ../ colgando.
target_include_directories(sandbox PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/sandbox)

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