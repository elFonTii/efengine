#pragma once
#include <efengine/renderer/Model.h>
#include <efengine/renderer/MaterialDef.h>
#include <optional>
#include <vector>

namespace efengine {
namespace resources {
    // Metros por unidad del archivo, a partir del UnitScaleFactor que declara su
    // metadata. assimp lo expresa al reves de lo que necesitamos: un archivo en
    // centimetros dice 100, y lo que queremos es 0.01.
    //
    // El caller pasa 0.0 cuando el metadata no trae la clave. Un factor <= 0 no
    // tiene interpretacion posible y dividir por el reventaria la escala entera,
    // asi que cae en 1.0 (o sea, no escalar).
    f32 MetersPerUnit(f64 unitScaleFactor);

    // El modelo y los materiales que declaraba el archivo, de un solo parseo.
    struct LoadedModel {
        renderer::Model                    model;
        std::vector<renderer::MaterialDef> materials;
    };

    class ModelLoader {
        public:
            static std::optional<LoadedModel> Load(const char* path);
    };
}
}