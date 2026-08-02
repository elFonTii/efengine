#include "NetCamera.h"
#include "Avatars.h"
#include "NetDebugUI.h"

#include <efenet/Client.h>
#include <efenet/Types.h>

#include <efengine/application/Application.h>
#include <efengine/resources/SceneAssets.h>
#include <efengine/serialization/SceneSerializer.h>
#include <efengine/serialization/SceneRegistry.h>
#include <efengine/serialization/BinaryReader.h>
#include <efengine/scene/Camera.h>
#include <efengine/scene/SceneGraph.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/Mesh.h>
#include <efengine/renderer/Vertex.h>
#include <efengine/core/Log.h>

#include <memory>
#include <vector>

namespace {
    using namespace efengine;

    // La escena del sandbox trae su piso como malla GENERADA, no como archivo:
    // el .efe guarda el id "sandbox.plane" mas sus params. Si el generador no
    // esta registrado, Resolve loguea un warning y el nodo queda sin malla, o
    // sea el demo sin piso. Por eso se replica acá.
    struct PlaneParams {
        f32 halfSize = 300.0f;
        f32 tiles    = 24.0f;

        template <class Ar>
        void Serialize(Ar& ar) {
            ar.Field(halfSize);
            ar.Field(tiles);
        }
    };

    renderer::Model makePlane(f32 halfSize, f32 tiles) {
        const std::vector<renderer::Vertex> vertices = {
            { {-halfSize, 0.0f, -halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f,  0.0f},  {1.0f, 0.0f, 0.0f} },
            { { halfSize, 0.0f, -halfSize}, {0.0f, 1.0f, 0.0f}, {tiles, 0.0f},  {1.0f, 0.0f, 0.0f} },
            { { halfSize, 0.0f,  halfSize}, {0.0f, 1.0f, 0.0f}, {tiles, tiles}, {1.0f, 0.0f, 0.0f} },
            { {-halfSize, 0.0f,  halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f,  tiles}, {1.0f, 0.0f, 0.0f} },
        };
        const std::vector<u32> indices = { 0, 3, 2, 2, 1, 0 };

        std::vector<renderer::Mesh> meshes;
        meshes.emplace_back(vertices, indices, "ground");
        return renderer::Model(std::move(meshes));
    }

    std::unique_ptr<renderer::Model> generarPlano(serialization::BinaryReader& r) {
        PlaneParams p;
        p.Serialize(r);
        if (!r.Ok()) return nullptr;
        return std::make_unique<renderer::Model>(makePlane(p.halfSize, p.tiles));
    }

    // Un cubo simple como avatar. Suficiente para el PoC: lo que se evalua es
    // el movimiento, no el modelo.
    renderer::Model hacerCubo(f32 lado) {
        const f32 h = lado * 0.5f;
        const std::vector<renderer::Vertex> vertices = {
            // cara frontal (+Z)
            { {-h, -h,  h}, {0,0,1}, {0,0}, {1,0,0} },
            { { h, -h,  h}, {0,0,1}, {1,0}, {1,0,0} },
            { { h,  h,  h}, {0,0,1}, {1,1}, {1,0,0} },
            { {-h,  h,  h}, {0,0,1}, {0,1}, {1,0,0} },
            // cara trasera (-Z)
            { { h, -h, -h}, {0,0,-1}, {0,0}, {-1,0,0} },
            { {-h, -h, -h}, {0,0,-1}, {1,0}, {-1,0,0} },
            { {-h,  h, -h}, {0,0,-1}, {1,1}, {-1,0,0} },
            { { h,  h, -h}, {0,0,-1}, {0,1}, {-1,0,0} },
        };
        const std::vector<u32> indices = {
            0,1,2, 2,3,0,     // frente
            4,5,6, 6,7,4,     // atras
            5,0,3, 3,6,5,     // izquierda
            1,4,7, 7,2,1,     // derecha
            3,2,7, 7,6,3,     // arriba
            5,4,1, 1,0,5,     // abajo
        };

        std::vector<renderer::Mesh> meshes;
        meshes.emplace_back(vertices, indices, "avatar");
        return renderer::Model(std::move(meshes));
    }

    // Muestrea el teclado y arma el comando de este tick.
    efenet::InputCmd muestrearInput(const platform::Input& in, f32 yaw, u32 seq) {
        efenet::InputCmd cmd;
        cmd.seq = seq;
        cmd.yaw = yaw;

        if (in.IsDown(platform::Key::W)) cmd.buttons |= efenet::Button::Forward;
        if (in.IsDown(platform::Key::S)) cmd.buttons |= efenet::Button::Back;
        if (in.IsDown(platform::Key::A)) cmd.buttons |= efenet::Button::Left;
        if (in.IsDown(platform::Key::D)) cmd.buttons |= efenet::Button::Right;

        return cmd;
    }
}

int main(int argc, char** argv) {
    using namespace efengine;

    // IP por argumento para poder probar entre dos maquinas. Sin argumento,
    // localhost: es el caso de dos ventanas en la misma PC.
    const char* host = (argc > 1) ? argv[1] : "127.0.0.1";

    EF_LOG_INFO("=== efenet: cliente -> %s:%u ===", host, static_cast<u32>(efenet::kPort));

    application::Application app;
    app.SetClearColor(0.10f, 0.12f, 0.16f);

    efenet::Client client;
    if (!client.Ok()) {
        EF_LOG_ERROR("efenet: no se pudo crear el host de red");
        return 1;
    }
    if (!client.Connect(host, efenet::kPort)) {
        EF_LOG_ERROR("efenet: no se pudo iniciar la conexion a %s", host);
        return 1;
    }

    // La escena del sandbox aporta piso, luces y entorno; los avatares se crean
    // encima en runtime. Los behaviors del sandbox no se registran a proposito:
    // los objetos decorativos quedan quietos, que para el demo de red da igual.
    resources::SceneAssets assets;
    scene::SceneGraph escena;
    serialization::SceneRegistry registry;
    registry.meshes.Register("sandbox.plane", &generarPlano);

    if (!serialization::SceneSerializer::Load("assets/scenes/sandbox.efe",
                                              escena, assets, app.GetResources(), registry)) {
        EF_LOG_ERROR("efenet: no se pudo cargar assets/scenes/sandbox.efe");
        return 1;
    }

    renderer::Model modeloAvatar = hacerCubo(1.0f);

    scene::Camera camara;
    camara.SetAspect(app.GetWindow().GetAspectRatio());

    netdemo::NetCamera netCamara(&camara);

    // El avatar necesita un material CONCRETO: con un MaterialMap vacio el
    // renderer no lo dibuja y loguea "sin material para malla 'avatar'" en cada
    // frame. Se reusa el primer material de la escena cargada — para el PoC lo
    // que importa es el movimiento, no el look.
    const renderer::Material* materialAvatar = assets.MaterialAt(0u);
    if (materialAvatar == null) {
        EF_LOG_ERROR("efenet: la escena no trajo materiales, el avatar no se puede dibujar");
        return 1;
    }
    netdemo::Avatars avatares(escena, &modeloAvatar,
                              renderer::MakeUniformMaterialMap({ "avatar" }, materialAvatar));

    std::vector<efenet::PlayerState> aDibujar;
    std::vector<efenet::PlayerState> remotos;

    f32 acumulador = 0.0f;
    u32 seq = 0u;

    while (app.Running()) {
        app.BeginFrame();

        if (app.IsKeyPressed(platform::Key::Escape)) app.Close();

        const f32 dt = app.DeltaTime();
        const platform::Input& in = app.GetInput();

        // El panel de ImGui se queda con el mouse cuando el cursor esta sobre el.
        netCamara.SetMouseEnabled(!app.GetDebugUI().WantsMouse());
        netCamara.Update(in, dt);

        // Recibir: CADA FRAME, no por tick. Adentro del while de abajo, a
        // 20 fps se llamaria rara vez y la latencia percibida subiria por un
        // detalle de estructura del loop, no por la red.
        client.Poll(dt);

        netdemo::DrawNetDebugUI(client, dt);

        // NETWORK LOOP: paso fijo de 60 Hz
        acumulador += dt;
        while (acumulador >= efenet::kTickDt) {
            const efenet::InputCmd cmd = muestrearInput(in, netCamara.Yaw(), ++seq);
            client.Tick(cmd);   // predice y envia con redundancia
            acumulador -= efenet::kTickDt;
        }

        // RENDER LOOP: tiempo continuo, a la tasa que de la maquina

        // Los remotos se dibujan kInterpDelay EN EL PASADO.
        const f32 renderTime = client.ServerTime() - efenet::kInterpDelay;
        client.SampleRemotes(renderTime, remotos);

        aDibujar.clear();
        if (client.MyId() != efenet::kInvalidPlayer) aDibujar.push_back(client.Predicted());
        for (const efenet::PlayerState& p : remotos) aDibujar.push_back(p);

        avatares.Sync(aDibujar);

        netCamara.Follow(client.Predicted().position);
        camara.SetAspect(app.GetWindow().GetAspectRatio());

        app.RenderScene(escena, camara);
        app.EndFrame();
    }

    EF_LOG_INFO("=== efenet: cliente cerrado ===");
    return 0;
}
