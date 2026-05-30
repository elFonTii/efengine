# sh/ — Scripts de compilación

Scripts de PowerShell para construir **efengine**. Todos usan build
*out-of-source* en `build/` y no ensucian el árbol de fuente.

Ejecútalos desde la raíz del repo.

## Requisitos

- **CMake** y **Visual Studio 18 2026** (toolset v145).
- **Python 3** con **jinja2** (`pip install jinja2`): GLAD 2.x genera el
  loader de OpenGL en tiempo de build con un script de Python. La primera
  compilación además descarga `gl.xml` de Khronos, así que necesita internet.

| Script | Qué hace |
|--------|----------|
| `configure.ps1` | Genera los proyectos VS en `build/`. |
| `build.ps1` | Compila (configura solo si hace falta). |
| `run.ps1` | Compila y ejecuta el `sandbox`. |
| `clean.ps1` | Borra `build/` (y artefactos in-source con `-InSource`). |
| `rebuild.ps1` | `clean` + `configure` + `build` desde cero. |

## Ejemplos

```powershell
.\sh\build.ps1                      # Debug
.\sh\build.ps1 -Config Release      # Release
.\sh\build.ps1 -Target efengine     # solo la librería
.\sh\run.ps1                        # compila y corre sandbox
.\sh\rebuild.ps1 -Config Release    # desde cero
.\sh\clean.ps1 -InSource            # limpia todo, incluso restos viejos
```

## Ajustes

Las variables compartidas (generador, arquitectura, carpeta de build)
están en `_config.ps1`.

> Si PowerShell bloquea la ejecución de scripts, en esa sesión:
> `Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass`
