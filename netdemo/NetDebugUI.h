#pragma once
#include <efengine/core/Types.h>

namespace efenet { class Client; }

namespace netdemo {

    // Panel de red del demo.
    //
    // Es lo que hace que el PoC demuestre algo: en localhost la latencia es ~0
    // y la prediccion es indistinguible de no tener prediccion. Con 200 ms de
    // latencia simulada y los cuatro toggles se ve exactamente que aporta cada
    // tecnica.
    void DrawNetDebugUI(efenet::Client& client, f32 dt);

}
