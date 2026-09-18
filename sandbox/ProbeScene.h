#pragma once

namespace sandbox {

    struct EditorContext;

    // Monta un .fbx cualquiera con un material gris plano y reporta al log lo
    // que hizo falta para montarlo: tiempo de carga, submallas, triangulos,
    // vertices, materiales distintos y AABB.
    //
    // Es una SONDA, no una escena. Existe para contestar "el motor, tal como
    // esta hoy, puede tragarse esto?" con numeros en vez de con opiniones. Sin
    // texturas a proposito: el loader no lee DDS, y un material plano aisla el
    // costo de la geometria del costo del sombreado.
    void BuildProbeScene(EditorContext& ctx, const char* fbxPath);

}
