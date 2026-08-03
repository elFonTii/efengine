#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/DdgiVolume.h>

namespace efengine {
namespace renderer {

    class Texture;
    struct DdgiSettings;

    // Lo que un frame necesita saber de DDGI. Espeja a IblContext y a
    // ShadowContext: punteros no-duenos al DdgiPass, que vive en Application.
    //
    // Si falta cualquiera de los dos atlas, o settings es null, MakeDdgiBlock
    // apaga params1.x y pbr.frag cae a IBL puro en vez de samplear una unidad
    // de textura sin contenido.
    struct DdgiContext {
        const Texture*      irradianceAtlas = null;
        const Texture*      distanceAtlas   = null;
        const DdgiSettings* settings        = null;
        UpdateRange         range           {};
    };

}
}
