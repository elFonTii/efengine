# Tests end-to-end estilo Gherkin y restricciones de calidad — análisis de viabilidad

Análisis pedido antes de escribir código (regla "plan antes de código"). Nada de
lo que sigue está implementado en el repo: es la propuesta, con la evidencia que
la sostiene y las decisiones que le tocan a la persona.

> **Veredicto corto:** es viable, y ya está demostrado en esta rama con un spike
> desechable (§2). Pero **no se puede hacer "tal cual está el motor"**: hacen
> falta seis cambios habilitantes chicos en `application`, `efecom` y `sandbox`
> (§4), y hay que aceptar que "verificar absolutamente todo" por E2E no es la
> meta correcta — la meta es que **cada pase, cada rasgo de la física y cada
> flujo del editor tenga al menos un escenario que falle si se rompe** (§3).

---

## 1. Qué se pidió y cómo se interpreta

| Pedido | Interpretación |
|---|---|
| E2E "al estilo Gherkin" | Archivos `.feature` legibles (en español: `Característica`, `Escenario`, `Dado`/`Cuando`/`Entonces`) con step definitions en C++. |
| "Headed, accediendo a efengine y corroborando" | Ventana GLFW real, contexto GL 4.5 real, `Application` real, `ScenePipeline` real. Nada de mocks de GPU. Se inspecciona el estado del motor y los píxeles que produce. |
| "Todas las funcionalidades" | Una matriz de cobertura explícita (§6) donde cada pase de `BuildFramePipeline`, cada pase de post, cada rasgo de física y cada flujo del editor tiene escenarios. Un test de CTest falla si aparece un pase sin `.feature` (§8.4). |
| "A fondo los pases y las físicas" | Tres niveles de oráculo para los pases (estructural, diferencial, golden) y física verificada contra analítica, invariantes y determinismo bit a bit. |
| "Restricciones de calidad estricta" | Reglas que hoy son prosa en el README pasan a ser **chequeos automáticos que rompen el CI** (§8). |

---

## 2. Evidencia: lo que ya se probó en este contenedor

No es una estimación en frío. En un Ubuntu 24.04 sin GPU:

1. **Contexto GL 4.5 Core sobre Xvfb + Mesa llvmpipe.**
   `glxinfo`: `OpenGL core profile version string: 4.5 (Core Profile) Mesa 25.2.8`,
   renderer `llvmpipe (LLVM 20.1.2, 256 bits)`. Compute shaders, texturas 3D de
   storage y timestamp queries funcionan.
2. **El sandbox compila y corre headed** en ese X virtual: arranca la escena
   `sandbox.efe`, voxeliza (256³, 96 MB), captura DDGI y presenta. La captura de
   pantalla muestra la sala de Cornell con el sangrado de color rojo/verde de la
   GI en el piso.
3. **Los 547 tests unitarios pasan** (`ctest`: 100 %, 1.84 s).
4. **Spike E2E en proceso** (un `main` alternativo linkeado contra los `.o` del
   sandbox, sin tocar el repo). Resultado real:

```
Característica: sala de Cornell
Escenario: el pipeline completo corre sin errores de GL
  PASS hay pases registrados              (10 — falta IBL, ver abajo)
  60 frames, 196.3 ms/frame de pared (llvmpipe, 1280x720, 4 cores)
  profiler soportado: si
    Sombras en cascada   gpu=6.46ms  draws=72
    Sombras              gpu=2.35ms  draws=18
    DDGI                 gpu=10.19ms draws=0
    AO                   gpu=47.49ms draws=21
    Indirecta            gpu=10.13ms draws=1
    Skybox               gpu=0.00ms  draws=0     <- no dibuja: no hay entorno
    Forward              gpu=44.36ms draws=18
    Bloom + tonemap      gpu=51.21ms draws=14
    FXAA                 gpu=15.65ms draws=1
    ...
Escenario: apagar AO no deja la pantalla vacia (depthReady)
  PASS AoPass se encuentra por tipo
  PASS sin errores con AO apagado
Escenario: una esfera cae y reposa sobre el piso (reloj sintetico)
  pose final: (0.000000, 0.480000, 0.000000)
  PASS la esfera quedo apoyada en y ~= radio
  PASS EndSimulation devuelve el local exacto
  PASS dos corridas dan la misma pose bit a bit
```

Lo que el spike enseñó, y que alimenta el resto del documento:

- **El acceso en proceso alcanza para casi todo.** `GetPipeline().Find<T>()`,
  `enabled`, `GetProfiler().stats().Rows()` (draws y ms por pase) y `GameWorld`
  ya exponen lo necesario para oráculos estructurales y de física.
- **El IBL no existe en el CI.** `assets/hdr/` está en `.gitignore` (decisión de
  `ddde828`), así que `IblPass` no se registra y el `SkyboxPass` corre con 0
  draws. Hoy **ningún test automático podría ver el IBL ni el skybox**. Hace
  falta un HDR chico versionado como fixture de test (§4, H5).
- **La esfera reposa en y = 0.48, no en 0.50.** Es el *penetration slop* de
  Jolt. Los escenarios de física tienen que declarar tolerancias explícitas y
  justificadas, no `≈`.
- **Determinismo bit a bit dentro del mismo binario: sí** (Jolt corre con
  `JobSystemSingleThreaded`). Entre plataformas/compiladores, no está garantizado
  sin `JPH_CROSS_PLATFORM_DETERMINISTIC`.
- **"Cero errores de GL" hoy es un chequeo vacío en Linux.** Ver §4, H4: el debug
  output solo se activa con `_DEBUG`, que GCC/Clang no definen.
- **llvmpipe es lento a 1280×720** (~5 fps). La resolución de E2E tiene que ser
  configurable (§4, H1); a 320×180 hay 16× menos píxeles y el grueso del costo
  (AO, forward, bloom) escala con píxeles.

---

## 3. Arquitectura propuesta

### 3.1 Tres formas de hacerlo, y cuál se recomienda

| Opción | Cómo | A favor | En contra |
|---|---|---|---|
| **A. Harness en proceso** (recomendada) | Ejecutable `efengine_e2e` que construye una `Application` real, carga escenas y ejecuta los steps llamando a la API del motor. | Acceso total al estado (`ScenePipeline`, profiler, `GameWorld`, `SceneGraph`). Determinista. Sin protocolo nuevo que mantener. Se integra con CTest. | No prueba "desde afuera" el ejecutable `sandbox` tal cual se distribuye. |
| B. Driver externo | `behave`/`pytest-bdd` en Python habla con el sandbox por un socket/JSON. | Caja negra pura. | Hay que diseñar y mantener un protocolo de automatización dentro del motor: superficie grande, frágil, y todo lo que no se exponga no se puede verificar. |
| C. Macros BDD de doctest | `SCENARIO`/`GIVEN`/`WHEN`/`THEN` (doctest ya está). | Cero dependencias. | No hay `.feature`: el Gherkin queda escrito en C++, que no es lo pedido. |

**Recomendación: A**, y para los flujos del editor sumar la *Dear ImGui Test
Engine* (§3.4), que maneja la UI real con mouse y teclado, visible en pantalla.

### 3.2 Runner de Gherkin

Dos caminos viables, **decisión de la persona**:

1. **[cwt-cucumber](https://github.com/ThoSe1990/cwt-cucumber)** — intérprete de
   Gherkin nativo en C++, sin Ruby; soporta Scenario Outline, Background, Rule,
   hooks, tablas y doc strings. **Requiere C++20.** Se podría subir el estándar
   *solo* del target `efengine_e2e` (el motor sigue en C++17). Antes de elegirlo
   hay que verificar dos cosas: que acepte las palabras clave en español
   (`# language: es`) y que conviva con la regla 6 (sin excepciones) — esto
   último solo afecta al target de tests, pero conviene saberlo.
2. **Runner propio mínimo en C++17** (`tests/e2e/runner/`). El subconjunto que
   hace falta es chico: `Característica`, `Antecedentes`, `Escenario`,
   `Esquema del escenario` + `Ejemplos`, `Dado/Cuando/Entonces/Y/Pero`, tags
   `@...` y tablas. Registro de steps por regex. Del orden de 500 líneas, sin
   dependencias, sin excepciones, y cada escenario se registra como un test de
   CTest (igual que `doctest_discover_tests`). Es territorio del agente según la
   tabla de "División de trabajo" (tests + tooling).

Recomendación: **2**, porque mantiene el proyecto entero en C++17, sin
excepciones, y el subconjunto de Gherkin necesario es estable. Si en algún
momento se quieren reportes en formato Cucumber JSON/JUnit, se agregan al runner.

### 3.3 Estructura

```
tests/e2e/
├── CMakeLists.txt            # target efengine_e2e, opción EFENGINE_BUILD_E2E (OFF por default)
├── runner/                   # parser Gherkin + registro de steps + integración CTest
├── harness/
│   ├── E2EWorld.h/.cpp       # dueño de Application, SceneGraph, SceneAssets, GameWorld, Camera
│   ├── FrameDriver.h/.cpp    # corre N frames con reloj sintético
│   ├── Capture.h/.cpp        # readback de targets (vía efecom) + métricas de imagen
│   └── Golden.h/.cpp         # comparación con tolerancia contra baselines por backend
├── steps/                    # step definitions agrupadas por dominio
│   ├── EscenaSteps.cpp  PasesSteps.cpp  FisicaSteps.cpp  EditorSteps.cpp  SerializacionSteps.cpp
├── features/
│   ├── pases/  fisica/  editor/  serializacion/  pipeline/
├── fixtures/                 # escenas .efe mínimas, HDR 64x32, modelos chicos
└── baselines/
    ├── llvmpipe/             # goldens del CI
    └── nvidia/               # goldens de la máquina de desarrollo
```

Un proceso por **archivo `.feature`**, una `Application` por proceso, y la escena
se reinicia entre escenarios con `SceneGraph::Clear()` (que ya sube la
generación y dispara la revoxelización, `86d75f7`). No se reconstruye la
`Application` por escenario: `Window` hace `glfwTerminate`, `GpuProfiler`
registra un puntero global (`SetActiveProfiler`) y Jolt admite un solo runtime
por proceso; recrear todo por escenario multiplica el riesgo de estado global
colgado y el tiempo de arranque.

### 3.4 El editor (ImGui)

Para escenarios como "apreto *Simular* en el editor y el cubo cae", la
herramienta hecha para eso es la **Dear ImGui Test Engine**: mueve el mouse,
hace click en widgets por su ID y se ve en pantalla (headed de verdad). Requiere
linkear el sandbox con hooks de la test engine. **Ojo con la licencia**: no es
MIT como ImGui; es gratuita para uso individual/no comercial y paga para
empresas por encima de cierto tamaño. Hay que leerla antes de adoptarla.
Alternativa sin licencia: steps que llamen directo a las funciones del editor
(`sandbox::BuildCornellScene`, `EndSimulationBeforeClear`, …) sin pasar por los
clicks — prueba la lógica del editor pero no el cableado de los botones.

### 3.5 Headed local y CI

| Dónde | Cómo | Para qué |
|---|---|---|
| Windows de desarrollo (RTX 3070) | `.\sh\e2e.ps1` abre la ventana real y corre las features; `-Slow` agrega pausa entre steps para mirarlos. | Headed "de verdad", baselines `nvidia/`, medición de rendimiento. |
| CI Linux (`ubuntu-latest`) | `xvfb-run` + Mesa llvmpipe (`LIBGL_ALWAYS_SOFTWARE=1`). La ventana existe en un X virtual. | Bloquea merges. Baselines `llvmpipe/`. Sube capturas y diffs como artefactos si falla. |
| Opcional: runner self-hosted con GPU | Tu máquina registrada como runner de GitHub. | Escenarios de rendimiento y goldens NVIDIA en CI. Llena las columnas vacías de `OPTIMIZACION.md` automáticamente. |

---

## 4. Cambios habilitantes en el motor (prerrequisitos)

Sin estos, los E2E serían frágiles o directamente imposibles. Todos son chicos.
Los marcados **(motor)** son código del motor: según la tabla del README, los
escribe o aprueba la persona. Los demás son build/tests/UI y caen del lado del
agente.

| # | Cambio | Por qué es necesario | Dónde |
|---|---|---|---|
| **H1** | `Application(const ApplicationDesc&)` con tamaño, vsync y título. El ctor por defecto sigue dando 1280×720 con vsync. | Hoy está fijo en `Application.cpp` (`WindowProps{ "efengine", 1280, 720, true }`). En llvmpipe 1280×720 cuesta ~196 ms/frame; el E2E necesita 320×180 o similar, y resoluciones fijas para que los goldens comparen. | (motor) `application/` |
| **H2** | Reloj inyectable: `BeginFrame()` que acepte un tiempo externo (o un `ClockSource`). | `BeginFrame` llama a `m_time.Tick()` con el reloj real. Con llvmpipe cada frame dura ~200 ms: `FixedSteps()` varía entre frames y se satura en 5 (`maxDelta` 0.1 s). La física E2E **tiene** que dar exactamente un paso fijo por frame. El spike lo esquivó con un `core::Time` aparte (`Time::Advance` ya existe y los tests unitarios lo usan). | (motor) `application/`, `core/Time` |
| **H3** | Readback en el RHI: `efecom::ReadPixels(framebuffer, x, y, w, h, formato, destino)` y lectura de textura/nivel. Getters de solo lectura para los targets intermedios de cada pase (shadow map, AO, atlas DDGI, HDR de escena). | No existe ninguna lectura de GPU a CPU en el RHI. Sin eso no hay oráculos de píxel. La regla 15 impide hacerlo con `gl*` desde el harness: **tiene** que entrar por `efecom`. | (motor) `efecom/RHI.h`, `RHIOpenGL.cpp`, pases |
| **H4** | Asserts y debug output de GL activados por la configuración de CMake, no por `_DEBUG`. | `EF_ASSERT`, `EFCOM_ASSERT`, el `GLFW_OPENGL_DEBUG_CONTEXT` y `glDebugMessageCallback` dependen de `#ifdef _DEBUG`. **Ese macro lo define MSVC en Debug; GCC y Clang no.** Consecuencia hoy: el job `Compilar (Debug)` del CI — cuyo comentario dice que existe "para destapar bugs que solo aparecen con _DEBUG activo" — compila **sin asserts y sin debug output**. Propuesta: `target_compile_definitions(... $<$<CONFIG:Debug>:_DEBUG>)` o, mejor, un `EF_ENABLE_ASSERTS` propio controlado por opción. El harness E2E instala un `MessageSink` que convierte cualquier `MessageSeverity::Error` en falla del escenario. | build + (motor) `core/Assert.h`, `efecom/Assert.h` |
| **H5** | Fixtures versionadas: un `.hdr` chico (64×32, unos KB) y escenas `.efe` mínimas bajo `tests/e2e/fixtures/`. | `assets/hdr/` está ignorado: sin fixture, IBL y skybox quedan fuera de toda verificación automática (confirmado en el spike). | tests |
| **H6** | Sacar de `sandbox/main.cpp` los behaviors (`RotarY`, `OrbitarXZ`, `RotarSolY`) y los generadores (`sandbox.plane`, `sandbox.box`) a una librería `sandbox_lib` que linkean el sandbox y el E2E. | `sandbox.efe` los referencia por nombre; hoy viven en un namespace anónimo de `main.cpp` y el E2E tendría que duplicarlos (el spike tuvo que copiar `generarCaja`). Además `kProbeModel` es una ruta absoluta de una máquina (`D:/@ffontana/...`). | build + sandbox |

Opcionales pero muy útiles:

- **Contratos de orden ejecutables.** Las restricciones de `FramePipeline.cpp`
  ("ShadowPass antes que DdgiPass", "IndirectPass después de FrameUploadPass",
  etc.) hoy viven en comentarios. Si cada pase declara qué publica y qué consume
  del `FrameContext` (una lista estática), un test unitario verifica el orden y
  el E2E lo reafirma. Esto convierte una familia entera de bugs en error de CI.
- **Hook de captura por pase** en `ScenePipeline::Execute` (callback opcional
  después de cada pase) para que un escenario pueda leer el target *justo después*
  de un pase concreto sin que el pase sepa nada del test.

---

## 5. Oráculos: cómo se "corrobora" un pase

Tres niveles. Cada pase tiene escenarios en los tres, del más robusto al más
frágil.

**N1 — Estructural** (no depende de píxeles, portable entre GPUs):
el pase está registrado y en el orden esperado; `enabled` lo apaga; emitió la
cantidad esperada de draws/dispatches (`GpuProfiler` por scope y
`efecom::GetFrameCounters()`); no hubo mensajes de error del driver; el
redimensionado llega a todos los pases, incluso deshabilitados.

**N2 — Diferencial / de propiedades** (la base del plan, robusto entre drivers):
se renderiza la misma escena con el pase encendido y apagado (o con un parámetro
en dos valores) y se compara una **métrica sobre una región**, no píxel a píxel.
Ejemplo: "con AO, la luminancia media del rincón es al menos 15 % menor".

**N3 — Golden con tolerancia** (pocas, una por escena canónica): captura
completa contra un baseline **por backend** (`llvmpipe/`, `nvidia/`), métrica
RMSE + porcentaje de píxeles fuera de umbral (o NVIDIA FLIP si se quiere
perceptual). Actualizar un baseline es un flag explícito
(`--update-baselines`) y el diff de la imagen se commitea: nunca se regenera en
silencio.

### 5.1 Matriz por pase (los 11 de `BuildFramePipeline` + post)

| Pase | N1 | N2 (métrica diferencial) |
|---|---|---|
| `CascadedShadowPass` | 4 cascadas, draws > 0 por cascada; draws bajan si se cullean casters | Píxel del piso bajo un bloque: más oscuro que el mismo material al sol; con el pase apagado la diferencia desaparece. Continuidad en el cambio de cascada al alejar la cámara. |
| `ShadowPass` (mapa de escena para DDGI) | draws > 0 con DDGI activo, sigue a DDGI (`68a4e0a`) | Irradiancia de una probe ocluida < probe expuesta. |
| `IblPass` | Se registra con el HDR fixture; publica `ctx.lighting.ibl.environment` | Rotar/cambiar el entorno cambia el tono de una esfera blanca rugosa. Esfera negra con IBL apagado. |
| `DdgiPass` (+ `VoxelizePass`) | Probes/atlas con las dimensiones del log; revoxeliza tras `Clear` (`86d75f7`); 1 voxel de margen (`4b326e6`) | **Sala de Cornell**: el piso junto a la pared roja tiene R/G mayor que con la ablación "irradiancia constante". Convergencia: la diferencia entre frames consecutivos del atlas decrece. |
| `AoPass` | prepass escribe depth y prende `depthReady` | Rincón más oscuro con AO. **Regresión documentada**: con AO apagado la imagen no queda vacía (el forward no debe usar `GL_EQUAL` contra el depth del frame anterior). |
| `FrameUploadPass` | Corre una vez, después de Shadow/IBL/DDGI/AO | Mover la cámara mueve el objeto en pantalla en la dirección esperada. |
| `IndirectPass` | Target a 1/2 res si el toggle está activo | Media resolución vs completa: diferencia por debajo de umbral. |
| `SceneTargetPass` | — | Con skybox apagado, un píxel sin geometría tiene el `clearColor`. |
| `SkyboxPass` | draws = 1 con entorno | Píxel de cielo ≈ muestra del HDR en esa dirección. |
| `ForwardPass` | draws = renderables visibles (culling) | Un cubo con `albedoTint` puro aparece con ese tono en el centro. |
| `DdgiDebugPass` | draws > 0 solo si está activo | Aparecen las esferas de probes. |
| Bloom + tonemap | 14 draws (del spike) | Emisivo brillante: anillo alrededor con más luminancia que sin bloom. Exposición mayor ⇒ media mayor (monotonía). Salida en [0, 1]. |
| FXAA | 1 draw, último eslabón al backbuffer | Borde diagonal: más valores intermedios con FXAA que sin él. |

Ejemplo de feature:

```gherkin
# language: es
@pase:AO
Característica: Oclusión ambiental

  Antecedentes:
    Dado el motor con una ventana de 320x180
    Y la escena "fixtures/cornell.efe"
    Y la cámara en (0, 2, -9) mirando a (0, 1.5, 0)
    Y que corrieron 30 frames

  Escenario: el AO oscurece los rincones
    Cuando capturo la imagen HDR con el pase "AO" encendido
    Y capturo la imagen HDR con el pase "AO" apagado
    Entonces la luminancia media de la región "rincon_izq" baja al menos un 15%

  Escenario: apagar el AO no deja la pantalla vacía
    Dado que el pase "AO" está apagado
    Cuando corren 3 frames
    Entonces menos del 1% de los píxeles de la imagen final tiene el color de limpieza
    Y el driver no reportó errores
```

---

## 6. Física: escenarios a fondo

Hoy la física tiene buena cobertura **unitaria** (`PhysicsWorld` 24 casos,
`PhysicsBinding` 17, `GameWorld` 6, `ShapeDesc` 9). Lo que falta es el camino
entero: `Application` + `GameWorld::Tick` con el `Time` del frame +
interpolación + render + editor. Propuesta:

| Área | Escenarios |
|---|---|
| Analítica | Caída libre: `y(t)` contra `h − ½gt²` dentro de la tolerancia del integrador de Jolt. Esfera apoyada en `y = r − slop` (medido: 0.48 para r = 0.5). |
| Reposo y apilado | Una pila de 5 cajas sigue en pie a los 10 s; ningún cuerpo penetra el piso más que el slop; la energía cinética tiende a 0. |
| Tipos de movimiento | Estático nunca se mueve; kinemático sigue al nodo y **empuja** a un dinámico; malla dinámica se degrada a estática (ya unitario, falta verlo en escena). |
| Jerarquía y escala | Hijo de un padre escalado: la forma absorbe la escala y el write-back da un local relativo al padre. Esfera con escala no uniforme se aproxima al eje dominante (el log lo avisa). |
| Ciclo de simulación | `EndSimulation` restaura los locals **exactos** (probado en el spike). Cargar otra escena durante la simulación la corta antes (`b8e9abf`). Destruir un nodo en pleno vuelo no deja cuerpos huérfanos. |
| Paso fijo | Con reloj sintético: 1 paso por frame a 60 Hz; con frames de 100 ms → 5 pasos y `FixedStepsSaturated()`; `Alpha()` coherente. |
| Interpolación | La posición *renderizada* está entre `prev` y `curr`; con readback (H3), el centro del objeto en pantalla coincide con la proyección de la pose interpolada (±2 px). |
| Determinismo | Dos corridas iguales en el mismo binario → poses idénticas bit a bit (probado). Hash de las poses cada 60 pasos como "huella" del escenario. Entre plataformas: solo si se activa `JPH_CROSS_PLATFORM_DETERMINISTIC` (decisión de la persona; tiene costo). |
| Bordes conocidos | Una esfera chica y rápida contra una caja fina: hoy no hay CCD expuesto en `ShapeDesc`/`BodyPose`, así que el escenario **documenta** el túnel esperado; si se agrega `LinearCast`, el escenario se invierte. |
| Huecos de API | `ShapeDesc` no expone fricción ni restitución: no se pueden escribir escenarios de rebote o deslizamiento hasta que existan. Queda anotado, no se inventa. |

```gherkin
# language: es
@fisica
Característica: Caída y reposo

  Esquema del escenario: una <forma> cae desde <altura> m y reposa
    Dado el motor con una ventana de 320x180 y reloj sintético a 60 Hz
    Y un piso estático de 8x8 m en y = 0
    Y una <forma> dinámica de radio 0.5 a <altura> m
    Cuando empiezo la simulación y corren 240 frames
    Entonces la <forma> reposa a y = <reposo> con tolerancia 0.005
    Y su velocidad es menor que 0.01 m/s
    Cuando termino la simulación
    Entonces la <forma> vuelve exactamente a <altura> m

    Ejemplos:
      | forma  | altura | reposo |
      | esfera | 3      | 0.48   |
      | esfera | 10     | 0.48   |
```

---

## 7. Viabilidad: costos y riesgos

| Riesgo | Impacto | Mitigación |
|---|---|---|
| llvmpipe lento | 196 ms/frame a 720p. Una feature de física de 240 frames tardaría ~47 s. | H1 (320×180). Frames de "calentamiento" compartidos por `Antecedentes`. Paralelizar features en CTest (`-j`), un proceso por feature. Estimación: suite completa en 5–10 min en CI. |
| Diferencias entre drivers | Goldens NVIDIA ≠ llvmpipe. | N2 como base; N3 solo con baselines por backend y pocas imágenes. |
| Flakiness temporal | El reloj real hace variar los pasos fijos. | H2 obligatorio. Prohibido `sleep` y tiempo de pared en steps. |
| Estado global | `glfwTerminate`, profiler global, un runtime de Jolt por proceso. | Una `Application` por proceso (§3.3). |
| Mantenimiento de los `.feature` | Se desactualizan si nadie los mira. | Regla de CI §8.4 (pase nuevo sin feature = rojo). Steps reutilizables y pocos (un catálogo, no uno por escenario). |
| "Todo" es infinito | Cobertura E2E exhaustiva es lenta y frágil. | Pirámide: la lógica sigue en unitarios (547 hoy); el E2E cubre integración, pases y flujos. |
| Windows sin CI | El CI es solo Linux; MSVC (tu toolchain) no se compila en ningún job. | Job `windows-latest` (build + unitarios). El E2E en Windows sin GPU es posible con Mesa para Windows, pero es opcional. |

Esfuerzo estimado, en fases (cada una con su plan aprobado antes):

| Fase | Contenido | Criterio de aceptación |
|---|---|---|
| F0 | H1–H6 | El spike de §2 se reescribe sin atajos: sin `core::Time` paralelo, sin copiar generadores, con IBL presente. |
| F1 | Runner Gherkin + harness + 3 features smoke + job de CI con `xvfb-run` | El job corre en cada push y sube capturas si falla. |
| F2 | Pases: N1 + N2 para los 11 pases de escena y el post | Todo pase de `BuildFramePipeline` tiene ≥ 1 escenario N1 y ≥ 1 N2. |
| F3 | Física (§6) | Tabla de §6 cubierta, determinismo con huella. |
| F4 | Goldens (N3), editor (ImGui Test Engine o steps directos), serialización con GPU (guardar → cargar → misma imagen) | Una golden por escena canónica por backend. |
| F5 | Restricciones de §8 en modo "ratchet" → estrictas | CI bloquea merges a `master`. |

---

## 8. Restricciones de calidad propuestas

El README ya tiene reglas MUST muy buenas; el problema es que **casi ninguna está
verificada por una máquina**. La propuesta es que cada regla tenga un guardián
automático, y que todo entre en modo *ratchet* (la deuda existente se congela y
no puede crecer; lo nuevo cumple desde el día uno).

### 8.1 Compilador

| Restricción | Estado actual (medido) | Propuesta |
|---|---|---|
| Warnings | **Ninguna flag de warnings** en ningún `CMakeLists.txt` del motor. Con `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wold-style-cast -Wdouble-promotion -Wnon-virtual-dtor -Woverloaded-virtual -Wnull-dereference -Wimplicit-fallthrough -Wformat=2`: **195 warnings únicos** en 113 archivos (97 `old-style-cast`, 62 `double-promotion`, 14 `unused-parameter`, 14 `sign-conversion`, resto sueltos). Por módulo: `efecom` 67, `sandbox` 66, `platform` 26, `renderer` 24, resto < 10. | Target `INTERFACE` `efengine_warnings` aplicado solo a targets propios (no a `_deps`). Primero warnings, después de limpiar (≈ 200 cambios mecánicos) `-Werror` / `/WX`. MSVC: `/W4 /permissive-`. |
| Regla 6 (sin excepciones) | El README dice "compila sin excepciones", pero **el CMake no lo impone**: no hay `-fno-exceptions` ni `/EHs-c-`. Probado: los 113 archivos compilan con `-fno-exceptions` sin cambios. | `-fno-exceptions` (GCC/Clang) y `/EHs-c- /D_HAS_EXCEPTIONS=0` (MSVC) en los targets del motor. Es gratis hoy y cierra la regla para siempre. |
| RTTI | `ScenePipeline::Find<T>` y `BehaviorRegistry` usan `typeid`. | Se mantiene RTTI; documentarlo como decisión. |
| Asserts en Debug | Ver H4: en Linux no se activan nunca. | H4. |
| Sanitizers | No hay. | Job `asan-ubsan` (Clang, Debug) corriendo los unitarios; el E2E con ASan es viable pero lento en llvmpipe: arrancar con los unitarios. |

### 8.2 Análisis estático (clang-tidy, mapeado a las reglas MUST)

`.clang-tidy` en la raíz con los checks que corresponden a cada regla, corriendo
sobre los archivos cambiados en cada PR:

| Regla del README | Check |
|---|---|
| 1–2 (RO0 / RO5 completo) | `cppcoreguidelines-special-member-functions` |
| 3–4 (sin `new`/`delete`, raw = observador) | `cppcoreguidelines-owning-memory`, `cppcoreguidelines-no-malloc`. Excepción documentada con `NOLINT(…)` para el idioma `unique_ptr<T>(new T)` con ctor privado (`IblPass`, `AoPass`, `DdgiPass`, `IndirectPass`, `VoxelizePass`, `PhysicsWorld`, `JoltRuntime`, `GameWorld`). |
| 5 (move `noexcept`) | `performance-noexcept-move-constructor` |
| 7 (assert sin efectos) | `bugprone-assert-side-effect` con `AssertMacros: EF_ASSERT,EF_ASSERT_MSG,EFCOM_ASSERT` — encaja exacto con la regla. |
| 12 (const-correctness) | `readability-make-member-function-const`, `misc-const-correctness` |
| General | `bugprone-*`, `performance-*`, `modernize-use-override`, `cppcoreguidelines-virtual-class-destructor` |

**Medido en este contenedor** (clang-tidy 18, 113 unidades de `src/` y `sandbox/`):

- **Hoy clang-tidy no puede ni arrancar en 103 de 113 unidades.** `core/Types.h`
  declara `constexpr nullptr_t null = nullptr;` sin `std::`. `<cstddef>` solo
  garantiza `std::nullptr_t`; que GCC y MSVC lo acepten es un detalle de su
  librería, y Clang lo rechaza. Arreglo de una línea (`std::nullptr_t`), y es
  condición previa para adoptar cualquier herramienta basada en Clang.
- Con ese parche aplicado solo en el análisis, la deuda es **chica: 22
  hallazgos**. 9 `performance-move-const-arg` (`std::move` que no mueve nada),
  3 `special-member-functions` (`ScopedMs` en `DdgiPass.cpp` y
  `VoxelizePass.cpp`, `CamposAlineados` en `AuthoringUI.cpp`: violan la regla
  2), 3 `unchecked-optional-access` (`PhysicsBinding.cpp:79`,
  `VertexArray.cpp:74`, `ColliderGizmo.cpp:97`), 2 multiplicaciones `u32` que
  se ensanchan después de desbordar (`GpuProfiler.cpp:26`,
  `ModelLoader.cpp:89`), un redondeo `(x + 0.5)` incorrecto para negativos
  (`ProfilerStats.cpp:24`), una comparación entre enums distintos
  (`EditorUI.cpp:189` — falso positivo: ImGui combina a propósito flags públicos
  y privados de docking; va con `NOLINT` y el motivo), 2 ramas duplicadas y un
  método que puede ser `const`. Los `unchecked-optional-access` probablemente
  estén protegidos por el llamador (p. ej. `addBody` solo se llama si el nodo
  tiene collider); ahí la corrección es un `EF_ASSERT` que documente la
  precondición (regla 9), no un `if`.
  `bugprone-assert-side-effect` y `owning-memory`: **cero** — las reglas 3, 4 y
  7 ya se cumplen, así que activarlas es gratis y solo evita que se rompan.

Conclusión: clang-tidy con estos checks puede entrar como **bloqueante casi de
inmediato** (arreglar `Types.h` + 22 hallazgos).

### 8.3 Linter de reglas propias (script, barato)

Lo que ni el compilador ni clang-tidy saben, en `tools/lint_rules.py`, corriendo
en CI:

- **Regla 15:** ningún `gl[A-Z]…(` ni `#include <glad/…>` fuera de `src/efecom/`
  (hoy se cumple: verificado).
- **Regla 6:** ningún `throw`, `try`, `catch` en `src/`.
- **Regla 14:** en líneas **agregadas** por el PR, ni `nullptr` ni `unsigned int`
  (hay 98 `nullptr` heredados; no se tocan, pero no crecen).
- **Regla 13:** todo `.cpp` en `src/` tiene su `.h` y viceversa.
- **Capas de módulos.** Grafo medido hoy:

  ```
  core          -> (nada)
  math          -> core
  platform      -> core
  physics       -> core math            (NO scene: lo exige ShapeDesc.h)
  renderer      -> core platform scene
  scene         -> core math platform renderer
  resources     -> core renderer
  serialization -> core math renderer resources scene
  gameplay      -> core math physics renderer scene
  application   -> core platform renderer resources scene
  efecom        -> (nada de efengine)
  ```

  Se congela como lista permitida: una arista nueva rompe el CI. Hay un ciclo
  **`renderer ↔ scene`** (9 pases de `renderer/` incluyen `scene/`, y `scene/`
  incluye `Bounds`, `Cull`, `Material`, `Model`, luces) que queda registrado
  como deuda conocida.
- **Sin rutas absolutas** en código (`D:/…` en `sandbox/main.cpp`).
- **Higiene del repo:** nada generado versionado. Hoy hay 151 archivos de caché
  en `src/graphify-out/` (2.1 MB) y `comp.spv`, `frag.spv`, `vert.spv` en la raíz.

### 8.4 Reglas de proceso que el CI hace cumplir

1. **Pase nuevo ⇒ feature nueva.** Un test enumera los pases de
   `BuildFramePipeline` y del `PostChain` y exige un `.feature` con el tag
   `@pase:<Name()>` para cada uno. Lo mismo para behaviors y generadores
   registrados en el sandbox.
2. **Bug corregido ⇒ test de regresión.** Los commits `revision R…` corrigen
   bugs concretos (margen de voxel, normal hacia el rayo, corte de simulación
   antes de reemplazar escena…); cada uno debería llegar con el unitario o el
   escenario que lo hubiera atrapado.
3. **Cobertura con trinquete.** `gcov`/`llvm-cov` sobre unitarios + E2E; umbral
   por módulo que solo puede subir (físicas y serialización altos; renderer
   medido con E2E).
4. **`master` protegido:** PR obligatorio y checks requeridos — build
   Debug/Release Linux, build MSVC, unitarios, E2E llvmpipe, lint de reglas,
   clang-tidy, sanitizers. Sin push directo.
5. **Formato:** `.clang-format` que capture el estilo actual y se verifique solo
   sobre líneas cambiadas (`git clang-format --diff`), para no generar un diff
   masivo de reformateo.
6. **Rendimiento como dato, no como gate** (en el runner con GPU): cada corrida
   E2E exporta ms por pase del `GpuProfiler`; eso llena las columnas vacías de
   `OPTIMIZACION.md` y detecta regresiones grandes (> 20 %) como advertencia.

---

## 9. Decisiones que le tocan a la persona

1. Runner: propio en C++17 (recomendado) o cwt-cucumber con C++20 solo en el
   target de E2E.
2. Aprobar H1–H4 (código del motor) y quién los escribe.
3. Dear ImGui Test Engine (licencia) o steps directos para el editor.
4. `JPH_CROSS_PLATFORM_DETERMINISTIC`: ¿se quiere determinismo entre Windows y
   Linux (huellas de física compartidas) a cambio de algo de rendimiento?
5. Runner self-hosted con la 3070 para goldens NVIDIA y rendimiento.
6. Orden de adopción de §8: sugerencia — `-fno-exceptions` y H4 ya (costo cero),
   lint de reglas y capas después, warnings como error cuando se limpien los 195.
