# EFENGINE - README.MD

## Lineamientos de desarrollo
- CONSISTENCIA: Cada clase deberá contar con su archivo .cpp y .h, no importa que sea header-only o una clase simple. 
- Estructura de Header: namespace - variables - funciones - macros - constantes
- Siempre incluir <glad/gl.h> ANTES de <GLFW/glfw3.h>
- Copy = delete


## Conceptos clave
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