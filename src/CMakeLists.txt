# src/CMakeLists.txt
# Define la librería efengine y sus dependencias.

# efecom (RHI) primero: efengine depende de él, nunca al revés.
add_subdirectory(efecom)
add_subdirectory(efengine)

# efenet: capa de red. Depende de efengine_core/math/binary, NUNCA de
# renderer/scene/platform: ese es el invariante que hace headless al servidor.
add_subdirectory(efenet)