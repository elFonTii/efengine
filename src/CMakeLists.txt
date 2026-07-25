# src/CMakeLists.txt
# Define la librería efengine y sus dependencias.

# efecom (RHI) primero: efengine depende de él, nunca al revés.
add_subdirectory(efecom)
add_subdirectory(efengine)