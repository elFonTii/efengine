#pragma once
#include <efengine/renderer/Model.h>
#include <efengine/core/Types.h>

#include <string>

// Generadores de mallas primitivas. Cada función arma la geometría de forma
// procedural (posiciones, normales, UVs y tangentes) y devuelve un Model de una
// sola malla con el materialName indicado, listo para el pipeline PBR.
//
// Requieren un contexto OpenGL vivo, porque Model->Mesh sube VBO/EBO al construir.
namespace efengine {
namespace renderer {
namespace primitives {

    // Plano XZ centrado en el origen, normal +Y. halfSize es la mitad del lado;
    // tiles escala las UVs para repetir la textura.
    Model Plane(const std::string& materialName, f32 halfSize = 0.5f, f32 tiles = 1.0f);

    // Cubo centrado en el origen. halfExtent es la mitad de la arista.
    Model Cube(const std::string& materialName, f32 halfExtent = 0.5f);

    // Esfera UV centrada en el origen. sectors = divisiones en longitud,
    // stacks = divisiones en latitud.
    Model Sphere(const std::string& materialName, f32 radius = 0.5f, u32 sectors = 32, u32 stacks = 16);

    // Cilindro con eje en Y, centrado en el origen, con tapas. height es la
    // altura total; sectors = divisiones radiales.
    Model Cylinder(const std::string& materialName, f32 radius = 0.5f, f32 height = 1.0f, u32 sectors = 32);

    // Cápsula con eje en Y, centrada en el origen: cilindro de altura height
    // rematado por dos semiesferas de radio radius. sectors = divisiones
    // radiales; rings = anillos por casquete.
    Model Capsule(const std::string& materialName, f32 radius = 0.5f, f32 height = 1.0f, u32 sectors = 32, u32 rings = 8);

}
}
}
