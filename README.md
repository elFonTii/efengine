# EFENGINE - README.MD

## Lineamientos de desarrollo

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
10. **RAII SIEMPRE** (Resource Acquisition Is Initialization): adquirir en el constructor, liberar en el destructor. Nada de `init()`/`shutdown()` manuales como API primaria.
11. **El orden de inicialización es contrato:** si un objeto depende del orden de construcción de sus miembros, decláralos en ese orden y documéntalo (ej. `Application`: `Window` antes que `Context`).
12. **const-correctness:** métodos que no mutan son `const`; parámetros de solo lectura no-propietarios van por `const&`.
13. **CONSISTENCIA — una clase, una responsabilidad, su `.h` + `.cpp`** (aunque sea header-only o trivial). Interfaz mínima y clara.

### D. Tipos / consistencia
14. **Usar siempre los alias de `Types.h`** (`u32`, `f32`, `usize`, …) y `null`; nada de tipos crudos (`unsigned int`) ni `nullptr` directo en código nuevo.

### Estilo
- Estructura de Header: namespace → variables → funciones → macros → constantes
- Siempre incluir `<glad/gl.h>` ANTES de `<GLFW/glfw3.h>`


## Conceptos clave
- Métodos estáticos: un método puede ser estático o devolver const si todo lo que se utiliza entra por parámetros, lo que significa que no depende de ningun objeto.
- VBOs -> Move datos de vertices (Vetex Data) una sóla vez pero se pueden reutilizar en cada frame. No guarda la información, guarda una referencia a la información de los arrays. (simplemente un bloque de memoria donde se guarda la info de los vértices)
- VAOs -> Es una lista de bytes, le dice a la GPU como escribir datos usando ese VBO
- EBOs -> Es un VBO pero usado con un propósito diferente. 
- MACROS:
 Un macro es una función que se expande en tiempo de compilación, lo que permite escribir código más limpio y fácil de usar.
 Ejemplo: EF_LOG_WARNING(fmt, ...) se expande luego a  ::efengine::core::Log(::efengine::core::LogLevel::Info, fmt, ##__VA_ARGS__)

- Tipos de MACROS:  
 1. Asserts: Macro que evalúa una condición y si es verdadera continua la ejecución, va a facilitar la vida cuando se quiera evaluar que el programa esté funcionando como se espera en un punto especifico, se pueden desactivar en la versión final. NO PUEDEN ALTERAR EL ESTADO DEL PROGRAMA, no es un condicional if.
 Assert = “esto DEBE ser verdadero, si no, error de programación”
 ```cpp
 // Ejemplo
int damage = CalculateDamage();
EF_ASSERT(damage >= 0, "Damage nunca debe ser negativo");
ApplyDamage(damage);
 ```

 ## Logging
 /src/efengine/core/Log.cpp
 Macros disponibles:
 - EF_LOG_DEBUG
 - EF_LOG_INFO
 - EF_LOG_WARNING
 - EF_LOG_ERROR

  ## Assertions
 /src/efengine/core/Assert.cpp
 Macros disponibles:
 - EF_ASSERT
 - EF_ASSERT_MSG