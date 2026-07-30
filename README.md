# efengine

Motor gráfico en **C++17 / OpenGL 4.5 Core**, construido desde cero como proyecto de largo plazo. Compila sin excepciones, sin `new`/`delete` crudos y con RAII en todos los subsistemas.

- **Ventanas e input:** GLFW 3.4
- **Loader GL:** GLAD 2 (generado en build time, `gl:core=4.5`)
- **Matemáticas:** GLM
- **Modelos:** Assimp (importador FBX, build mínimo estático)
- **Texturas:** stb_image
- **UI de tooling:** Dear ImGui (rama docking, DockSpace/DockBuilder)
- **Tests:** doctest + CTest

---
### 2026-30-07
IBL + DDGI
<img width="1916" height="1026" alt="image" src="https://github.com/user-attachments/assets/e50cc748-67d5-40ce-b03f-daafa469b616" />

### 2026-26-07
<img width="1919" height="1031" alt="image" src="https://github.com/user-attachments/assets/5144fd21-405d-4fd4-ac80-02e6f254d224" />

---

## Estructura del proyecto

```
efengine/
├── src/
│   ├── efecom/                  # RHI: la única superficie que habla con la GPU
│   │   ├── RHI.h                # API sin tipos de OpenGL a la vista (handles u32 opacos)
│   │   └── RHIOpenGL.cpp        # backend GL 4.5 (mañana puede haber otro)
│   └── efengine/
│       ├── core/                # Types.h (u32, f32, usize, null...), Log, Assert, Time
│       ├── platform/            # Window (GLFW), Input, InputCodes, IEventListener
│       ├── math/                # Math, Transform
│       ├── renderer/            # Renderer, Shader, Texture, Material/MaterialDef,
│       │                        # Buffer/VertexArray/IndexBuffer/VertexLayout, Mesh, Model,
│       │                        # Framebuffer, Cubemap, Environment (IBL), Bounds,
│       │                        # PointLight, DirectionalLight,
│       │                        # ShadowMap/ShadowPass/ShadowMath,
│       │                        # SkyboxPass, PostChain + TonemapPass/BloomPass/FxaaPass
│       ├── scene/              # SceneGraph, Node, NodeHandle, Behavior, Camera, CameraController
│       ├── resources/          # ResourceManager, ModelLoader, MaterialBuilder, SceneAssets, FileIO
│       ├── serialization/      # formato .efe: EfeFile, BinaryReader/Writer, StringTable,
│       │                        # SceneDocument, SceneSerializer, Behavior/MeshGenerator registries
│       └── application/        # Application (bundle RAII + frame API), DebugUI (ImGui)
├── sandbox/                    # Editor/ejemplo: main.cpp, EditorUI, AuthoringUI, AssetCatalog, FrameStats
├── tests/                      # Tests unitarios (doctest), espejando la estructura de src/
├── assets/                     # Shaders GLSL, escenas .efe, modelos FBX, texturas PBR, HDRIs
├── tools/                      # tiny-binary-inspector.html (hexdump de .efe en el navegador)
├── sh/                         # Scripts de PowerShell para configurar/compilar/correr/testear
└── cmake/                      # Doxyfile.in (target opcional `docs`) + CopyAssets.cmake
```

`efengine` es una librería estática compuesta por un target por módulo (`efengine_core`, `efengine_renderer`, …) y se consume como `<efengine/...>`. **`efecom` no depende de `efengine`** — usa sus propios alias en `efecom/Types.h`. La dependencia va en un solo sentido: `efengine → efecom`.

---

## División de trabajo

Este proyecto se desarrolla en pareja: una persona y un agente.

**Harness:** [Claude Code](https://claude.com/claude-code) (CLI + extensión de VSCode), con modelos **Opus** y **Fable** según la tarea — Opus para diseño, planes y trabajo largo; Fable para ejecución mecánica y de alto volumen.

| Quién | Qué |
|---|---|
| **Persona** (dueño del proyecto) | Escribe el **código del motor**. **Guía todas las decisiones técnicas**: arquitectura, alcance, orden del roadmap, y el sí/no final sobre cualquier propuesta. |
| **Agente** (Claude Code) | Escribe **planes y especificaciones** antes de tocar código. Mantiene el **motor de serialización** (`.efe`), el **tooling** (scripts de `sh/`, inspector binario), el **linking y el build** (CMake, targets, dependencias), los **tests** (doctest/CTest) y la **UI** (paneles ImGui del sandbox). |

Reglas que hacen que esto funcione:

- **Plan antes de código.** Nada grande se implementa sin una spec y un plan por fases revisados y aprobados.
- **Los principios MUST de abajo no son negociables** y se aplican igual a lo que escribe cualquiera de los dos.
- **Fase por fase se decide quién escribe el C++**, salvo en las áreas de la tabla, que ya tienen dueño.
- **Los shaders GLSL** los escribe el agente; el diseño del pipeline lo decide la persona.

---

## Cómo compilar

Requisitos: **CMake ≥ 3.20**, un compilador C++17, **Python 3 + jinja2** (`pip install jinja2`, GLAD genera el loader en build time) e internet en la primera compilación (FetchContent descarga todas las dependencias; no hay submódulos ni vendoring).

### Windows (scripts incluidos)

```powershell
.\sh\configure.ps1                  # genera proyectos de VS en build/
.\sh\build.ps1                      # compila (Debug por defecto)
.\sh\build.ps1 -Config Release
.\sh\run.ps1                        # compila y ejecuta el sandbox
.\sh\test.ps1                       # corre los tests
.\sh\rebuild.ps1                    # limpio + configure + build
```

> Al **agregar un `.cpp` nuevo** a un `CMakeLists.txt` hay que volver a correr `configure.ps1`: compilar con `--target` no dispara la reconfiguración y el archivo nuevo queda fuera del build (aparece como `LNK2019`).

### CMake pelado (cualquier plataforma)

```bash
cmake -S . -B build
cmake --build build
./build/bin/sandbox
```

Opciones:

| Opción | Default | Descripción |
|---|---|---|
| `EFENGINE_BUILD_TESTS` | `ON` | Compila los tests (doctest + CTest). Apagable si se embebe el motor. |

Con Doxygen instalado, `cmake --build build --target docs` genera la documentación en `docs/html`.

---

## Reglas de construcción

Principios **MUST** del motor: reglas no negociables que aplican a todo código nuevo.

### A. Memoria y propiedad

1. **Rule of Zero (RO0) por defecto.** Una clase NO declara destructor/copy/move salvo que posea *directamente* un recurso crudo; los miembros (valor, `unique_ptr`, contenedores) se automanejan.
2. **RO5 (Rule of Five) completo SOLO para "handle-owners"** (clases que poseen un handle de OS/GPU crudo, como `Window`, `Texture`, `Cubemap`): destructor + copy `= delete` + move `noexcept`. Todo o nada, nunca a medias.
3. **Prohibido `new`/`delete` crudos.** Propiedad = valor o `unique_ptr` (dueño único). `shared_ptr` solo ante propiedad genuinamente compartida y justificada.
4. **Punteros/referencias crudos = SOLO observadores no-propietarios.** Nunca liberan ni asumen propiedad. Un raw pointer jamás es dueño.
5. **Move siempre `noexcept`** y deja el origen vacío y válido (`std::exchange`).

### B. Errores (sin excepciones)

6. **El motor compila sin excepciones:** nada de `throw`/`try`/`catch` en runtime.
7. **Errores de programación → `EF_ASSERT`** (precondiciones e invariantes). Un assert NUNCA tiene efectos secundarios (no-op en release).
8. **Fallos esperados/recuperables → valor de retorno** (`bool`, `std::optional`, tipo expected-style). El llamador decide; no se aborta.
9. **Valida en la frontera, confía dentro:** chequea entradas en los límites públicos del módulo; dentro asume invariantes vía assert.

### C. Diseño / API

10. **RAII SIEMPRE:** adquirir en el constructor, liberar en el destructor. Nada de `init()`/`shutdown()` manuales como API primaria.
11. **El orden de inicialización es contrato:** si un objeto depende del orden de construcción de sus miembros, se declaran en ese orden y se documenta (ej. `Application`: `Window` → `Context` → `Framebuffer` → `Renderer` → `ResourceManager`).
12. **const-correctness:** métodos que no mutan son `const`; parámetros de solo lectura no-propietarios van por `const&`.
13. **CONSISTENCIA — una clase, una responsabilidad, su `.h` + `.cpp`** (aunque sea header-only o trivial). Interfaz mínima y clara.

### D. Tipos / consistencia

14. **Usar siempre los alias de `Types.h`** (`u32`, `f32`, `usize`, …) y `null`; nada de tipos crudos (`unsigned int`) ni `nullptr` directo en código nuevo.

### E. GPU

15. **Nada de llamadas `gl*` fuera de `efecom`.** El módulo `renderer` habla con la GPU solo por `efecom::RHI`; si falta una operación, se agrega al RHI, no se filtra OpenGL.

### Estilo

- Estructura de header: namespace → variables → funciones → macros → constantes.
- Siempre incluir `<glad/gl.h>` **antes** de `<GLFW/glfw3.h>` (solo dentro de `efecom`/`platform`).

---

## La API actual

La referencia de uso completa es [`sandbox/main.cpp`](sandbox/main.cpp): registra los tipos serializables, **carga la escena entera desde un `.efe`**, y corre un loop con editor ImGui, cámara con mouselook y behaviors animando nodos.

### 1. `Application`: punto de entrada

`Application` es un bundle RAII de los subsistemas (`Window` → `Context` → `Framebuffer` → `Renderer` → `ResourceManager`, en ese orden por contrato) e incluye la cadena de post-proceso, el skybox, el shadow pass y la UI de debug. Construirla levanta todo; destruirla lo baja. El loop principal vive en el cliente:

```cpp
application::Application app;
app.SetClearColor(0.18f, 0.18f, 0.18f);

while (app.Running()) {
    app.BeginFrame();
    if (app.IsKeyPressed(platform::Key::Escape)) app.Close();

    const platform::Input& in = app.GetInput();
    controller.Update(in, app.DeltaTime());

    scene.Update(app.DeltaTime());        // behaviors
    app.RenderScene(scene, cam);          // shadow → escena a HDR FBO → post → backbuffer
    app.EndFrame();
}
```

Frame API: `Running()`, `BeginFrame()`, `EndFrame()`, `RenderScene(scene, camera)`, `DeltaTime()` (f32, segundos), `Elapsed()` (f64), `IsKeyPressed(Key)`, `Close()`, `SetClearColor(r, g, b, a)`.
Accessors: `GetWindow()`, `GetRenderer()`, `GetResources()`, `GetTime()`, `GetInput()`, `GetDebugUI()`, `GetBloomPass()`, `GetFxaaPass()`, `GetShadowPass()`.

### 2. `SceneGraph`: jerarquía por handles con generación

La escena es un **grafo de nodos** con padre/hijos, no una lista plana. `NodeHandle` es un handle índice + generación: destruir un nodo invalida sus handles viejos (`IsValid` los detecta en lugar de leer basura).

```cpp
scene::SceneGraph scene;

const scene::NodeHandle lamp = scene.CreateChild(scene.Root(), "lamp");
scene.SetLocalTransform(lamp, transform);
scene.AttachMesh(lamp, { lampModel, lampMats });

const scene::NodeHandle sun = scene.CreateNode("sun");
scene.AttachLight(sun, { scene::LightKind::Directional, glm::vec3(3.0f) });
scene.SetPrimarySun(sun);

scene.Update(dt);   // corre behaviors y recalcula transforms world (solo lo dirty)
```

`UpdateWorldTransforms()` recorre el árbol propagando `worldMatrix` y de paso junta la vista de salida del frame: `Renderables()` (lista de `RenderItem{world, model, materials}`), `PointLights()` y `Sun()`.

Un `Node` tiene `local` (transform relativo al padre), `worldMatrix`, `parent`/`children`, y attachments opcionales: `mesh`, `light`, y un vector de `behaviors`.

### 3. `Behavior`: lógica por nodo

Un behavior es una clase con `OnUpdate(UpdateContext&)`. El contexto trae el grafo, el nodo, `dt` y `SetLocal()` (que marca el subárbol dirty). Si además define `Serialize`, se guarda y se restaura con la escena:

```cpp
class RotarY : public scene::Behavior {
    public:
        RotarY() = default;
        explicit RotarY(f32 degPerSec) : m_degPerSec(degPerSec) {}

        void OnUpdate(scene::UpdateContext& ctx) override {
            math::Transform t = ctx.node.local;
            t.rotation.y += ctx.dt * m_degPerSec;
            ctx.SetLocal(t);
        }

        template <class Ar>
        void Serialize(Ar& ar) { ar.Field(m_degPerSec); }

    private:
        f32 m_degPerSec = 0.0f;
};

scene.AttachBehavior(handle, std::make_unique<RotarY>(20.0f));
```

### 4. Serialización: el formato `.efe`

Escenas completas (jerarquía, transforms, materiales, mallas, luces y estado de behaviors) van a un **binario propio, versionado y chunkeado**. La escena del sandbox se carga de disco, no se arma a mano:

```cpp
serialization::SceneRegistry registry;
registry.behaviors.Register<RotarY>("RotarY");          // nombre estable en el archivo
registry.meshes.Register("sandbox.plane", &generarPlano); // malla procedural + su payload

resources::SceneAssets assets;   // dueño de materiales y mallas generadas
scene::SceneGraph scene;

if (!serialization::SceneSerializer::Load("assets/scenes/sandbox.efe",
                                          scene, assets, rm, registry)) { ... }

serialization::SceneSerializer::Save("assets/scenes/sandbox.efe",
                                     scene, assets, rm, registry);
```

Cómo está armado:

- **Header:** `magic` (`EFE1`) + `endianCheck` + `version` + `contentType` + `chunkCount`.
- **Chunks** con `id` (FourCC) + `byteSize` padeado a 4: `STRT` (tabla de strings), `SCNE` (settings), `MATL` (materiales), `NODE` (nodos en pre-orden, así `parent < i`). Un chunk desconocido se saltea por `byteSize`.
- **Un solo `Serialize(Ar&)` por tipo** sirve para leer y escribir: `BinaryWriter` y `BinaryReader` cumplen la misma interfaz (`Field`, `Array`, `Count`, `Version`, `Ok`).
- **Reglas de compatibilidad explícitas:** chunk nuevo → no sube versión; campo nuevo al final de un record o cambio de significado → sube `kCurrentVersion`. Se lee desde `kMinSupportedVersion`; una versión futura se rechaza. Abrir y guardar migra el archivo.
- **El reader es hostil por defecto:** todo `Count` valida contra el tamaño mínimo de elemento, así que un archivo truncado o corrupto falla en lugar de reservar gigabytes (ver `tests/serialization/Truncation.test.cpp`).

Para mirar un `.efe` a mano hay [`tools/tiny-binary-inspector.html`](tools/tiny-binary-inspector.html).

### 5. `ResourceManager` y `SceneAssets`: quién posee qué

`ResourceManager` es dueño de lo que viene **de disco por path** (shaders, texturas, modelos) y cachea por clave. Devuelve **punteros observadores** (regla 4): el llamador nunca libera, y un puntero nulo señala fallo de carga (regla 8, sin excepciones).

```cpp
resources::ResourceManager& rm = app.GetResources();

renderer::Shader*  pbr    = rm.GetShader("pbr", "assets/shaders/pbr.vert", "assets/shaders/pbr.frag");
renderer::Model*   rat    = rm.GetModel("assets/models/street_rat_4k.fbx");
renderer::Texture* albedo = rm.GetTexture("assets/textures/.../diff_4k.jpg", renderer::ColorSpace::sRGB);
```

Detalle importante: `GetTexture` recibe el `ColorSpace`. El albedo va en **sRGB**; normal, roughness, metallic, AO y height van en **Linear**.

`SceneAssets` es dueño de lo que **no** tiene path: los materiales de la escena y las mallas generadas por código. Guarda cada material como `MaterialDef` (la descripción serializable) + el `Material` armado. `UpdateMaterial` reemplaza el contenido de un slot **sin mover su dirección**, así los `const Material*` que ya guardaron los nodos no se enteran de la edición — eso es lo que hace posible editar materiales en vivo desde la UI.

### 6. `Material` y `MaterialDef`

`Material` en runtime solo tiene punteros a `Shader` y hasta siete `Texture` (albedo, normal, AO, roughness, metallic, height, opacity) más los escalares. `MaterialDef` es la **fuente de verdad para guardar**: nombre, shader, paths de cada mapa con su color space, y los escalares (`albedoTint`, `metallic`, `roughness`, `aoStrength`, `heightScale`, `alphaCutoff`). `MaterialBuilder` arma un `Material` desde un `Def` resolviendo texturas contra el `ResourceManager`.

Los índices de `TextureSlot` **son parte del formato**: no se reordenan sin subir la versión.

### 7. `Model`, `Mesh` y `MaterialMap`

Un `Model` es una lista de `Mesh`, y cada mesh lleva un `materialName`. Un `MaterialMap` (`std::unordered_map<std::string, const Material*>`) decide qué material usa cada nombre:

```cpp
renderer::MaterialMap ratMats = {
    { "street_rat",      &streetRatMat },
    { "street_rat_hair", &streetRatMat },
};

// o barrer todos los meshes de un modelo:
renderer::MaterialMap lampMats;
for (const renderer::Mesh& mesh : lamp->meshes())
    lampMats[mesh.materialName()] = &lampMat;
```

También se puede construir geometría a mano con `Vertex` (posición, normal, uv, tangente) e índices, como el plano del suelo del sandbox — y si se registra como generador, viaja en el `.efe` como nombre + payload en vez de como vértices.

### 8. Pipeline de render

`RenderScene` corre, en orden:

1. **Shadow pass** — profundidad de la escena desde el sol a un shadow map (2048² por default), con caja ortográfica y bias ajustables por ImGui (`ShadowSettings`).
2. **Escena** a un framebuffer **HDR (`RGBA16F`)**: PBR con luces puntuales (`kMaxLights = 4`, tiene que coincidir con el shader), luz direccional, sombras e **IBL difuso**.
3. **Skybox** con el cubemap de entorno.
4. **`PostChain`**: los pases se encadenan con ping-pong entre dos framebuffers scratch y el último escribe al backbuffer. Hoy: **bloom** (brightpass + blur + composite) → **tonemap** → **FXAA**.
5. **ImGui** encima.

**IBL:** `Environment::Create` toma un `.hdr` equirectangular, lo proyecta a un cubemap `RGBA16F` y lo convoluciona a un mapa de irradiancia difusa, ambos con **compute shaders** (`assets/shaders/ibl/`). Todo eso es precómputo: en el frame solo se samplea.

### 9. Cámara e input

`platform::Input` es el listener de la ventana y expone estado por frame — incluyendo **edge detection** (`WasPressed`), que es lo que hace que un tecleo no se dispare en varios frames seguidos. `Camera` + `CameraController` implementan órbita, pan, zoom y **mouselook con cursor capturado**; el controller consume el `Input` en lugar de recibir callbacks:

```cpp
scene::Camera cam;
cam.SetAspect(app.GetWindow().GetAspectRatio());
scene::CameraController controller(&cam);

controller.SetMouseEnabled(!app.GetDebugUI().WantsMouse());     // ImGui gana el mouse
controller.SetKeyboardEnabled(!app.GetDebugUI().WantsKeyboard());
controller.Update(in, app.DeltaTime());
app.GetWindow().SetCursorCaptured(controller.WantsCursorCaptured());
```

### 10. Editor del sandbox

`sandbox/` es un editor chico sobre ImGui con layout dockeado tipo Unity: jerarquía, inspector, panel de materiales, panel de render (bloom/FXAA/sombras/IBL), stats de frame, y autoría — spawnear modelos desde un catálogo de disco, editar materiales en vivo, y guardar/cargar `.efe`.

Su estado (`EditorState`) vive en `main` y se le presta al editor por `EditorContext`: **el loop es el dueño, la UI solo lee y escribe**. Nada de estado global.

### 11. Logging y asserts

```cpp
EF_LOG_DEBUG(...)  EF_LOG_INFO(...)  EF_LOG_WARNING(...)  EF_LOG_ERROR(...)

EF_ASSERT(cond)
EF_ASSERT_MSG(cond, "mensaje")   // errores de programación; no-op en release
```

---

## Tests

Tests unitarios con **doctest**, integrados a CTest y espejando la estructura de `src/` (`tests/core`, `tests/math`, `tests/renderer`, `tests/scene`, `tests/resources`, `tests/serialization`). Lo que se testea es lo que no necesita GPU: matemática de transforms y sombras, layouts de vértices, bounds, handles y ciclo de vida del grafo, behaviors, y **el formato `.efe` completo — round-trip, tabla de strings, registries y archivos truncados**.

```bash
cmake --build build
ctest --test-dir build
# o en Windows: .\sh\test.ps1
```

---

## Assets del sandbox

Los shaders (`assets/shaders/`) son propios. Los modelos, texturas PBR y HDRIs de ejemplo (street rat, industrial pipe lamp, brown mud, rock) son assets de terceros usados solo para la demo.
