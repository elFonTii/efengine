#pragma once
#include <efengine/renderer/Material.h>
#include <efengine/renderer/MaterialDef.h>
#include <efengine/resources/ResourceManager.h>

#include <optional>

namespace efengine {
namespace resources {
    // funcion libre, se declara en header para dejarla explicita
    std::optional<renderer::Material> BuildMaterial(const renderer::MaterialDef& def,
                                                    ResourceManager& rm);

}
}