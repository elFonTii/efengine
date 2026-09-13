# Optimización de pases — registro de ejecución

Registro de la orden de trabajo sobre los pases del renderer. Un commit por
tarea, en el orden del plan.

> **Las columnas de medición están vacías a propósito.** La implementación se
> hizo sin acceso a la GPU objetivo: no se midió ni un solo frame. Todas las
> ganancias que se citan son las estimadas por el plan o derivadas del código,
> nunca observadas. Rellenar la tabla es el siguiente paso y es lo único que
> puede confirmar o desmentir cada cambio.

---

## 1. Cómo medir

Antes de tomar el primer número:

```
sudo nvidia-smi -pm 1
sudo nvidia-smi -lgc 1500,1500
```

A ~4 ms de frame la 3070 entra y sale de boost constantemente y los deltas
quedan enterrados en el ruido. Al terminar: `sudo nvidia-smi -rgc`.

Media de ≥300 frames en estado estable, no el valor instantáneo del overlay.

**Casi todos los cambios tienen interruptor en caliente**, así que medir "con" y
"sin" no necesita recompilar — que importa, porque entre dos compilaciones
cambia el estado térmico de la GPU:

| Tarea | Interruptor |
|---|---|
| 0 | Render ▸ DDGI ▸ Diagnóstico ▸ *Ablation: irradiancia constante* |
| 1 | Render ▸ DDGI ▸ Rendimiento ▸ *Indirecta a media resolucion* |
| 2 | Render ▸ Oclusión ambiental ▸ Rendimiento ▸ *Media resolucion* |
| 4 | Render ▸ Oclusión ambiental ▸ *Habilitado* (apagarlo desactiva el prepass y el Forward vuelve a `GL_LESS`) |

Las tareas 3, 5 y 6 no tienen toggle: son reescrituras del pase, y sostener los
dos caminos en paralelo habría costado más de lo que aporta poder compararlos.
Para esas, la comparación es contra el commit anterior.

El panel de profiling ahora trae una columna **Peor ms** por pase y el overlay
muestra **p99** y conteo de picos. Ese es el instrumento de la tarea 7.

---

## 2. Registro de resultados

| # | Tarea | Estado | GPU antes | GPU después | CPU antes | CPU después | Frame | Commit |
|---|---|:--|---:|---:|---:|---:|---:|---|
| 0 | Ablation DDGI | ✅ implementado | 1.892 | | | | | `4391e30` |
| 1 | Indirecta media res | ✅ implementado | | | | | | `8c2407e` |
| 2 | SSAO media res | ✅ implementado | 1.249 | | | | | `12419d4` |
| 3 | Blend DDGI | ✅ implementado | 0.361 | | | | | `d21d6fb` |
| 4 | Depth `GL_EQUAL` | ✅ implementado | | | | | | `d52153f` |
| 5 | Captura instanciada | ✅ implementado | 0.150 | | 2.270 | | | `d56c5dd` |
| 6 | Fusión tonemap | ✅ implementado | 0.043 | | | | | `01ed8c0` |
| 7 | Picos de frametime | ✅ instrumentado | | | | | | `1110624` |

"Implementado" no es "verificado". Ninguna fila tiene medición.

---

## 3. Notas y hallazgos

### Tarea 0 — ablation test

Pendiente de ejecutar. La lectura es binaria:

- Forward cae a ~0.3 ms → la hipótesis P1 (latencia de memoria en el sampleo del
  volumen) queda confirmada y la tarea 1 es la respuesta correcta.
- Forward baja poco → el cuello está en otro lado. Toca mirar ocupancia y
  razones de stall de warp en Nsight **antes** de seguir. La tarea 1 ya está
  implementada, pero si el ablation no confirma, su ganancia va a ser marginal y
  el costo en calidad (media resolución) deja de justificarse: el toggle permite
  apagarla.

El valor constante del ablation arranca en 0.25 y no en 0 a propósito. Con cero,
el compilador puede plegar la multiplicación por albedo y borrar trabajo que el
camino real sí hace, y la medición mentiría a favor del ablation.

### 🔴 La premisa de P3 es falsa

El plan dice que el blend de DDGI "despacha el compute sobre el volumen completo
y actualiza el 0.5% de él". **No es así.** `DdgiPass.cpp` ya despachaba
`DispatchCompute(m_range.count, 1, 1)`: catorce workgroups, uno por probe
actualizado. No había 2880 probes de trabajo descartado, y la solución que el
plan proponía —compactar en un SSBO y usar dispatch indirecto— habría sido un
no-op.

El coste real, derivado del código:

```
irradiancia: 14 probes × (10×10 texels) × 6·16·16 muestras =  2.150.400
distancia:   14 probes × (18×18 texels) × 6·16·16 muestras =  6.967.296
                                                    total  =  9.117.696
```

Nueve millones de iteraciones, cada una con un `texelFetch`, repartidas entre
**catorce** workgroups. Un workgroup corre entero en un SM: de los 46 de la 3070
quedan 32 sin trabajo, y los 14 que reciben corren ~1 warp (irradiancia) o ~3
(distancia) contra las 48-64 que hacen falta para tapar la latencia de un fetch.
El pase no hacía trabajo de más — hacía el trabajo justo con la GPU casi vacía.

Lo que se hizo en su lugar: cache de la captura en shared memory (los 1536
texels se leían 100 y 324 veces por probe) y corte del lóbulo en el blend de
distancia (con exponente 50, la masa de peso por debajo de coseno 0.8 es 1.1e-5
del total).

**Lo que queda pendiente**, y es lo que puede hacer la diferencia si la medición
no llega: la ocupancia sigue capada en 14 workgroups. Arreglarla pide partir la
integral de cada probe entre 6 workgroups —uno por cara— y reducir en un segundo
dispatch, lo que necesita un scratch en SSBO o una imagen RGBA32F que el RHI
todavía no tiene. Es una reescritura del pase; el plan pide medir antes de
encadenar cambios y por eso no se hizo a ciegas.

### 🔴 La premisa de P2 también, en parte

El plan atribuye los 2.270 ms de CPU de la captura a "un `glBindFramebuffer` +
`glClear` por cara: 84 bind/clear por frame". **No los había**: el pase ya hacía
un solo bind y un solo clear antes del bucle, y cambiaba de cara con
`glViewport`. Los 2.27 ms eran los 588 draws en sí — cada uno con aplicar estado,
subir el `MaterialBlock`, hasta 8 `glBindTextureUnit` y subir la matriz de
objeto, unas 6.500 llamadas `gl*` por frame que a ~350 ns dan justo esa cifra.

Y el nivel 1 que el plan propone —layered rendering escribiendo `gl_Layer`— **no
aplica**: el target de captura no es un cubemap, es un atlas 2D de 96×512. No hay
capas donde escribir.

Lo que sí aplica, y es mejor: como es un atlas 2D, se puede ir directo al nivel 2
sin pasar por el 1. Un draw instanciado por objeto, cada instancia con su vista
sacada de un SSBO, transformada a su rectángulo del atlas con una escala y un
offset en clip space y recortada a él con `gl_ClipDistance`. **588 → 7 draws.**

`gl_ViewportIndex` habría sido la alternativa obvia y no sirve: GL garantiza 16
viewports y hacen falta `probes × 6` (84 por defecto). Los planos de recorte son
8 garantizados y hacen falta 4, así que el límite deja de depender de cuántos
probes se actualicen por frame.

### 🟡 La tarea 4 era "hacer", no "verificar"

El plan pide *verificar* que el Forward reutiliza el depth del AO prepass. No lo
reutilizaba: el prepass escribía en el renderbuffer de su propio framebuffer y el
Forward en el del framebuffer de escena. Dos buffers distintos, el overdraw
resuelto dos veces.

Al implementarlo aparecieron dos cosas que el plan no menciona y sin las cuales
el cambio se rompe:

1. **`gl_Position` tiene que coincidir bit a bit.** `GL_EQUAL` compara
   profundidades exactas. Hacen falta `invariant gl_Position` en los dos vertex
   shaders **y** la misma expresión: `ao/depth_normal.vert` tenía
   `uProjection * viewPos` teniendo `viewPos` ya calculado, que agrupa
   `P·((V·M)·p)` contra el `((P·V)·M)·p` de `pbr.vert`. `invariant` garantiza que
   la *misma* expresión dé lo mismo; no puede salvar dos expresiones distintas.
   El modo de fallar es total: la geometría desaparece a parches.

2. **El prepass tiene que descartar por opacidad.** `pbr.frag` descarta con
   `alpha < alphaCutoff`; si el prepass dejara profundidad donde el Forward no
   pinta, con `GL_EQUAL` eso taparía lo que hubiera detrás. Un recorte de follaje
   se volvería un agujero opaco con forma de quad. De paso arregla el AO, que
   ocluía con el quad entero.

Queda un caso conocido sin cubrir: un material con mapa de altura **y** de
opacidad a la vez. `pbr.frag` desplaza la UV con parallax antes de leer la
opacidad y el prepass no, así que la cobertura puede diferir en un píxel en el
borde del recorte. Meter POM en el prepass costaría más de lo que arregla.

### 🟡 Un bug propio, encontrado y corregido dentro de la tarea 2

La tarea 1 dejó los dos upsamples atados a un solo flag. Con el AO a media
resolución y el `IndirectPass` apagado, `pbr.frag` leía el target del AO con
coordenadas de resolución completa: el cuadrante superior izquierdo estirado
sobre toda la pantalla. Los dos flags son independientes ahora, y el prepass-guía
se mudó a `AoContext` —que es quien lo produce— para que la validez de cada
upsample la decida un solo lugar con los dos contextos en la mano.

### 🟡 Lo que el plan pedía verificar en el AO, verificado

- **Early-out de cielo**: ya existía. `if (viewZ <= 0.0)` sale antes del kernel.
- **Blur separable**: ya lo era. Dos draws, `uCounts.z` elige H o V.
- **Ruido en textura 4×4**: **no se hizo, a propósito.** `gtao.frag` usa
  interleaved gradient noise, que para este caso es mejor: cero fetches, mejor
  distribución, y no ata el blur a un tamaño de kernel. La receta "ruido 4×4 +
  blur 4×4" es del SSAO clásico y no aplica a un GTAO con blur bilateral.

### ⚪ Tarea 7 — lo que se descartó leyendo el código

No se puede investigar un pico sin medirlo, y el panel solo mostraba el promedio
—que por construcción esconde exactamente lo que un pico es. Lo entregado es el
instrumento. Descartado por lectura, para no volver a mirarlo:

- **`GpuProfiler` no estanca la CPU.** Pregunta `TimestampAvailable` antes de
  cada `TimestampNanos` y saltea el frame si el slot no está listo. El pool de
  consultas se aloca entero en el constructor.
- **Nada realoca en estado estable.** `Framebuffer::Resize`,
  `DdgiPass::EnsureAtlasSize` y `AoPass::EnsureTargetSize` salen temprano si el
  tamaño no cambió.
- **El reinicio del barrido de probes** —el sospechoso que el plan nombra— no
  aloca nada: solo deja de forzar histéresis 0 cuando el primer barrido completa.

Queda como candidato, y solo se confirma midiendo, el estado de boost de la GPU.
El propio plan anticipa que parte de los picos puede desaparecer al bloquear
relojes.

---

## 4. Deuda conocida que dejan estos cambios

- **La presión de registros de `pbr.frag` no baja.** El camino inline que samplea
  el volumen sigue compilado porque `IndirectPass` depende del prepass del AO: con
  el AO apagado es lo único que hay. El compilador reserva para la rama peor
  aunque el flag del UBO sea uniforme. Sacarlo del todo pide un sistema de
  permutaciones de shader que el motor no tiene, y es lo que habría que hacer si
  la hipótesis secundaria de P1 (baja ocupancia por registros) resulta ser la
  dominante.
- **Ocupancia del blend de DDGI**, descrita arriba.
- **El scratch entre el composite y FXAA sigue siendo RGBA16F** llevando datos
  LDR. `Framebuffer` no acepta formato hoy; el día que lo acepte, ese scratch es
  el primer candidato a RGBA8.
- **La captura de probes sigue sin culling por probe.** Con la escena de prueba
  no importa; cuando crezca, multi-draw indirect sobre el mismo SSBO de tiles es
  el camino natural y ya no pide RHI nuevo.
