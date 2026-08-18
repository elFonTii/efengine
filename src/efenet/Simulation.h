#pragma once
#include <efenet/PlayerState.h>
#include <efenet/InputCmd.h>

namespace efenet {

    // EL CORAZON DEL POC.
    //
    // Pura: mismos argumentos, mismo resultado. Sin estado global, sin leer el
    // reloj, sin aleatoriedad. El servidor authoritative y la prediccion del
    // cliente llaman LITERALMENTE a esta funcion; si dejan de coincidir, la
    // reconciliacion empieza a corregir en cada snapshot y se ve el temblor.
    //
    // 'dt' es SIEMPRE kTickDt, nunca el dt del frame. Predecir con el dt
    // variable del render mientras el servidor avanza a paso fijo hace que los
    // dos lados divergan desde el primer segundo, y es dificil de ver una vez
    // metido.
    //
    // No hace falta determinismo bit-exacto entre maquinas: el servidor manda y
    // la reconciliacion corrige, asi que alcanza con coincidir aproximadamente.
    // (Lockstep si lo necesitaria; por eso se descarto ese enfoque.)
    void Step(PlayerState& state, const InputCmd& input, f32 dt);

}
