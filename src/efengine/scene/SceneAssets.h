#pragma once

#include <efengine/core/Types.h>
#include <efengine/renderer/Material.h>
#include <efengine/renderer/MaterialDef.h>
#include <efengine/renderer/Model.h>

#include <memory>
#include <string>
#include <vector>

namespace efengine {
namespace scene {
    // Scene assets es dueña de los assets que no se cargan en la escena
    // los materiales y las mallas que son procedurales (generación de terreno, plane por codigo)

    class SceneAssets {
        public:
            // Mismo valor que en serialization
            static constexpr u32 kInvalidIndex = 0xFFFFFFFFu; // valor centinela

            u32 AddMaterial(renderer::MaterialDef def, renderer::Material mat);
            u32 MaterialCount() const { return static_cast<u32>(m_materials.size()); }

            const renderer::MaterialDef* DefAt(u32 index) const;
            const renderer::Material* MaterialAt(u32 index) const;
            u32 IndexOfMaterial(const renderer::Material* mat) const;
            
        private:
            struct MaterialSlot {
                renderer::MaterialDef               def;
                std::unique_ptr<renderer::Material> material;
            };

            struct GeneratedSlot {
                std::string                      name;
                std::vector<u8>                  payload;
                std::unique_ptr<renderer::Model> model;
            };

            std::vector<MaterialSlot>  m_materials;
            std::vector<GeneratedSlot> m_generated;
    };
}
}