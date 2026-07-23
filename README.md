# efengine

Motor gráfico en **C++17 / OpenGL 4.5 Core**, construido desde cero como proyecto de largo plazo. Compila sin excepciones, sin `new`/`delete` crudos y con RAII en todos los subsistemas.

- **Ventanas e input:** GLFW 3.4
- **Loader GL:** GLAD 2 (generado en build time)
- **Matemáticas:** GLM
- **Modelos:** Assimp (importador FBX, build mínimo estático)
- **Texturas:** stb_image
- **Tests:** doctest + CTest

---

## Estructura del proyecto

```
efengine/
├── src/efengine/
│   ├── core/         # Types.h (u32, f32, usize, null...), Log, Assert, Time
│   ├── platform/     # Window (GLFW), InputCodes, IEventListener
│   ├── math/         # Math, Transform
│   ├── renderer/     # Context, Renderer, Shader, Texture, Material,
│   │                 # Buffer/VertexArray/IndexBuffer, Mesh, Model, PointLight
│   ├── scene/        # Scene, SceneObject, Camera, CameraController
│   ├── resources/    # ResourceManager (caché de shaders/texturas/modelos), FileIO
│   └── application/  # Application (bundle RAII de subsistemas + frame API)
├── sandbox/          # Ejecutable de ejemplo (main.cpp — la referencia de uso de la API)
├── tests/            # Tests unitarios (doctest), espejando la estructura de src/
├── assets/           # Shaders GLSL (pbr, phong), modelos FBX y texturas PBR
├── sh/               # Scripts de PowerShell para configurar/compilar/correr
└── cmake/            # Doxyfile.in (target opcional `docs`)
```

---

## Cómo compilar

Requisitos: **CMake ≥ 3.20**, un compilador C++17, **Python 3 + jinja2** (`pip install jinja2`, GLAD genera el loader en build time) e internet en la primera compilación (FetchContent descarga todas las dependencias; no hay submódulos ni vendoring).

### Windows (scripts incluidos)

```powershell
.\sh\configure.ps1                  # genera proyectos de VS en build/
.\sh\build.ps1                      # compila (Debug por defecto)
.\sh\build.ps1 -Config Release
.\sh\run.ps1                        # compila y ejecuta el sandbox
.\sh\rebuild.ps1                    # limpio + configure + build
```

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
2. **RO5 (Rule of Five) completo SOLO para "handle-owners"** (clases que poseen un handle de OS/GPU crudo, como `Window`): destructor + copy `= delete` + move `noexcept`. Todo o nada, nunca a medias.
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
11. **El orden de inicialización es contrato:** si un objeto depende del orden de construcción de sus miembros, se declaran en ese orden y se documenta (ej. `Application`: `Window` → `Context` → `Renderer` → `ResourceManager`).
12. **const-correctness:** métodos que no mutan son `const`; parámetros de solo lectura no-propietarios van por `const&`.
13. **CONSISTENCIA — una clase, una responsabilidad, su `.h` + `.cpp`** (aunque sea header-only o trivial). Interfaz mínima y clara.

### D. Tipos / consistencia

14. **Usar siempre los alias de `Types.h`** (`u32`, `f32`, `usize`, …) y `null`; nada de tipos crudos (`unsigned int`) ni `nullptr` directo en código nuevo.

### Estilo

- Estructura de header: namespace → variables → funciones → macros → constantes.
- Siempre incluir `<glad/gl.h>` **antes** de `<GLFW/glfw3.h>`.

---

## La API actual

La referencia de uso completa es [`sandbox/main.cpp`](sandbox/main.cpp): carga dos modelos FBX con materiales PBR, un plano de suelo generado a mano, tres luces puntuales y una cámara orbital, y anima una luz y la rotación de un objeto en el loop.

### 1. `Application`: punto de entrada

`Application` es un bundle RAII de los subsistemas (`Window` → `Context` → `Renderer` → `ResourceManager` → `Time`, en ese orden por contrato). Construirla levanta todo; destruirla lo baja. El loop principal vive en el cliente:

```cpp
application::Application app;
app.SetClearColor(0.18f, 0.18f, 0.18f);

while (app.Running()) {
    app.BeginFrame();
    if (app.IsKeyPressed(platform::Key::Escape)) app.Close();

    // ... actualizar escena con app.DeltaTime() / app.Elapsed() ...

    app.RenderScene(scene, cam);
    app.EndFrame();
}
```

Frame API disponible: `Running()`, `BeginFrame()`, `EndFrame()`, `RenderScene(scene, camera)`, `DeltaTime()` (f32, segundos), `Elapsed()` (f64), `IsKeyPressed(Key)`, `Close()`, `SetClearColor(r, g, b, a)`. Accessors: `GetWindow()`, `GetRenderer()`, `GetResources()`, `GetTime()`.

### 2. `ResourceManager`: carga y caché de recursos

Único dueño de shaders, texturas y modelos. Devuelve **punteros observadores** (regla 4): el llamador nunca libera, y un puntero nulo señala fallo de carga (regla 8, sin excepciones).

```cpp
resources::ResourceManager& rm = app.GetResources();

renderer::Shader* pbr = rm.GetShader("pbr", "assets/shaders/pbr.vert", "assets/shaders/pbr.frag");
renderer::Model*  rat = rm.GetModel("assets/models/street_rat_4k.fbx");
renderer::Texture* albedo = rm.GetTexture("assets/textures/.../diff_4k.jpg", renderer::ColorSpace::sRGB);

if (!pbr || !rat || !albedo) {
    EF_LOG_ERROR("No se pudieron cargar los recursos");
    return 1;
}
```

Detalle importante: `GetTexture` recibe el `ColorSpace`. El albedo va en **sRGB**; normal, roughness, AO y height van en **Linear**.

### 3. `Material`: parámetros PBR sobre un shader

Un `Material` referencia (no posee) un shader y hasta siete mapas: albedo, normal, AO, roughness, metallic, height y opacity. Además expone escalares públicos ajustables:

```cpp
renderer::Material mat(pbr);
mat.SetAlbedoMap(albedo);
mat.SetNormalMap(normal);
mat.SetRoughnessMap(rough);
mat.SetAOMap(ao);

mat.metallic    = 0.8f;
mat.roughness   = 1.0f;
mat.heightScale = 0.0f;   // desactiva parallax si el mapa no se quiere usar
// también: albedoTint, aoStrength, alphaCutoff
```

Las funciones de armado propias del cliente que pueden fallar devuelven `std::optional<Material>` (ver `makePbrMaterial` en el sandbox, que resuelve texturas por convención de nombre `<base>{diff,nor_gl,rough,ao,disp}_<res><ext>`).

### 4. `Model`, `Mesh` y `MaterialMap`

Un `Model` es una lista de `Mesh`, y cada mesh lleva un `materialName`. Al armar la escena, un `MaterialMap` (`std::unordered_map<std::string, const Material*>`) asigna qué material usa cada nombre:

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

También se puede construir geometría a mano con `Vertex` (posición, normal, uv, tangente) e índices, como el plano del suelo en el sandbox (`makePlane`).

### 5. `Scene`: objetos y luces por handle

`Scene` guarda `SceneObject`s (modelo + materiales + `Transform`) y luces puntuales. `Add`/`AddLight` devuelven un handle `u32` para mutar el elemento después:

```cpp
scene::Scene scene;
scene.ambientFactor = 0.08f;

math::Transform ratTransform;
ratTransform.scale = glm::vec3(10.0f);
const u32 ratHandle = scene.Add({ rat, ratMats, ratTransform });

const u32 sun = scene.AddLight({ glm::vec3(50.0f, 80.0f, 0.0f), glm::vec3(5000.0f) });

// en el loop:
scene.GetLight(sun).position = glm::vec3(50.0f * std::cos(elapsed), 80.0f, 50.0f * std::sin(elapsed));
scene.Get(ratHandle).transform.rotation.y += app.DeltaTime() * 20.0f; // grados/seg
```

### 6. Cámara e input por eventos

`Camera` + `CameraController` implementan una cámara orbital. El controller es un `IEventListener` que se registra en la ventana y recibe resize, movimiento de mouse, botones y scroll:

```cpp
scene::Camera cam;
cam.SetAspect(app.GetWindow().GetAspectRatio());

scene::CameraController controller(&cam);
app.GetWindow().SetEventListener(&controller);
```

Soporta `SetInvertX/SetInvertY`, límites de zoom y rotación arrastrando con el mouse.

### 7. Logging y asserts

Macros de `core/`:

```cpp
EF_LOG_DEBUG(...)  EF_LOG_INFO(...)  EF_LOG_WARNING(...)  EF_LOG_ERROR(...)

EF_ASSERT(cond)
EF_ASSERT_MSG(cond, "mensaje")   // errores de programación; no-op en release
```

---

## Tests

Tests unitarios con **doctest**, integrados a CTest y espejando la estructura de `src/` (`tests/core`, `tests/math`, `tests/renderer`, ...).

```bash
cmake --build build
ctest --test-dir build
# o en Windows: .\sh\test.ps1
```

---

## Assets del sandbox

Los shaders (`assets/shaders/pbr.*`, `phong.*`) son propios. Los modelos y texturas de ejemplo (street rat, industrial pipe lamp, brown mud, rock) son assets PBR de terceros usados solo para la demo.
