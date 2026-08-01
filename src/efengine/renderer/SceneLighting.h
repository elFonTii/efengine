#pragma once
#include <efengine/renderer/ShadowContext.h>
#include <efengine/renderer/IblContext.h>
#include <efengine/renderer/DdgiContext.h>
#include <efengine/renderer/AoContext.h>

namespace efengine {
namespace renderer {

    // Los cuatro contextos de iluminacion del frame, juntos. Existe porque
    // BeginScene ya tenia ocho parametros y el AO era el noveno: cada sistema
    // nuevo sumaba uno mas a una firma que nadie podia leer de un vistazo.
    //
    // Los cuatro son punteros no-duenos a cosas que viven en Application.
    struct SceneLighting {
        ShadowContext shadow;
        IblContext    ibl;
        DdgiContext   ddgi;
        AoContext     ao;
    };

}
}
