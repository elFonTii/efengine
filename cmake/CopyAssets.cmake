# cmake/CopyAssets.cmake
# Sincroniza assets/ hacia el directorio del ejecutable copiando SOLO lo que cambió.
#
# Se invoca en modo script (cmake -P), no se incluye desde un CMakeLists:
#   cmake -DASSETS_SRC=<dir> -DASSETS_DST=<dir> -P cmake/CopyAssets.cmake
#
# Por qué no `cmake -E copy_directory`: ese comando reescribe todo el árbol en
# cada build. Con 145 MB de texturas y modelos eso son varios segundos por
# compilación. file(COPY) en cambio preserva el timestamp del origen y descarta
# los archivos que ya existen en destino con ese mismo timestamp, así que una
# build sin cambios en assets/ cuesta un stat por archivo (milisegundos).

cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED ASSETS_SRC OR NOT DEFINED ASSETS_DST)
    message(FATAL_ERROR "CopyAssets.cmake requiere -DASSETS_SRC=<dir> -DASSETS_DST=<dir>")
endif()

if(NOT IS_DIRECTORY "${ASSETS_SRC}")
    message(FATAL_ERROR "No existe el directorio de assets: ${ASSETS_SRC}")
endif()

file(GLOB_RECURSE _src_files RELATIVE "${ASSETS_SRC}" "${ASSETS_SRC}/*")
file(GLOB_RECURSE _dst_files RELATIVE "${ASSETS_DST}" "${ASSETS_DST}/*")

# Borrar del destino lo que ya no existe en origen: ni copy_directory ni
# file(COPY) borran nada, así que sin esto los assets renombrados o eliminados
# se acumulan indefinidamente junto al ejecutable.
set(_removed 0)
foreach(_rel IN LISTS _dst_files)
    if(NOT _rel IN_LIST _src_files)
        file(REMOVE "${ASSETS_DST}/${_rel}")
        math(EXPR _removed "${_removed} + 1")
    endif()
endforeach()

# La barra final hace que se copie el CONTENIDO de assets/ y no la carpeta
# dentro del destino (que daría .../assets/assets).
file(COPY "${ASSETS_SRC}/" DESTINATION "${ASSETS_DST}")

if(_removed GREATER 0)
    message(STATUS "assets: ${_removed} archivo(s) obsoleto(s) eliminado(s)")
endif()
