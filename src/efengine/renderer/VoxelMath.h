#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/Bounds.h>

#include <glm/glm.hpp>

namespace efengine {
namespace renderer {

    inline constexpr u32 kVoxelResolution = 256u;

    struct VoxelGridDesc {
        glm::vec3 origin     { 0.0f };              // esquina minima en mundo
        f32       voxelSize  { 1.0f };              // metros
        u32       resolution { kVoxelResolution };  // voxeles por eje
    };

    // Cubo centrado en la escena que la contiene entera. El grid es CUBICO
    // aunque la escena no lo sea: un grid anisotropo obligaria al DDA a llevar
    // tres tamanos de paso, y el ahorro de memoria no paga esa complejidad.
    //
    // Una AABB invalida devuelve un grid unitario en el origen. No se asserta
    // porque WorldBounds() de una escena vacia es Empty(), y cargar el editor
    // sin escena no es un error del llamador.
    VoxelGridDesc FitVoxelGrid(const AABB& worldBounds, u32 resolution);

    glm::vec3  VoxelToWorld(const VoxelGridDesc& g, glm::ivec3 voxel);  // CENTRO del voxel
    glm::ivec3 WorldToVoxel(const VoxelGridDesc& g, glm::vec3 world);
    bool       InsideGrid  (const VoxelGridDesc& g, glm::ivec3 voxel);
    AABB       GridBounds  (const VoxelGridDesc& g);
    f32        GridExtent  (const VoxelGridDesc& g);  // metros por lado

}
}
