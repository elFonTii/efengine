#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/AoContext.h>
#include <efengine/renderer/Framebuffer.h>
#include <efengine/renderer/IndirectContext.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/UniformBuffer.h>

#include <glm/glm.hpp>
#include <optional>

namespace efengine {
namespace renderer {

    class Renderer;
    class Shader;
    class Texture;
    class VertexArray;

    // Resuelve la irradiancia indirecta de DDGI a resolucion REDUCIDA en un
    // target propio, para que pbr.frag no tenga que samplear el volumen por
    // pixel.
    //
    // -- Por que --
    // El sampleo de DDGI son ~16 gathers por pixel con coordenadas calculadas
    // por pixel: 8 probes trilineales, y de cada uno un lookup octaedrico de
    // irradiancia mas los momentos de distancia para Chebyshev. Las direcciones
    // no son predecibles, no hay prefetch posible y la cache de texturas se
    // satura. En el Forward a 1080p eso medía 1.892 ms con SEIS draws: ~9.300
    // ciclos de shader por pixel, cuando un PBR con una direccional y sombra
    // ronda 200-500. No es ALU, no es ancho de banda y no es geometria.
    // DdgiSettings::ablateSample es el instrumento que lo confirma.
    //
    // A un cuarto de los pixeles son un cuarto de los gathers. Se puede porque
    // la indirecta difusa es de baja frecuencia espacial por construccion -- la
    // grilla ya la muestrea cada varios metros --, y lo que si tiene alto
    // detalle (albedo, AO de contacto) se sigue aplicando a resolucion completa.
    //
    // -- Orden --
    // Corre DESPUES de BeginScene y ANTES del skybox y el forward. Es el unico
    // pase que va ahi, y no es arbitrario:
    //
    //   - despues de BeginScene, porque asi la view/proj y la posicion de camara
    //     le llegan por el bloque Frame ya subido, y los dos atlas de DDGI ya
    //     estan en sus unidades. Repetir todo eso en un bloque propio seria una
    //     segunda fuente de verdad para la camara del frame.
    //   - despues del AoPass, porque LEE su prepass (posicion y normal) y su
    //     target (bent normal). No re-rasteriza la geometria: no hay un segundo
    //     prepass ni un G-buffer nuevo.
    //   - antes del forward, obviamente, porque el forward lo consume.
    //
    // -- Dependencia del AO --
    // Sin el prepass del AO este pase no tiene de donde sacar posicion ni
    // normal, asi que con el AO apagado NO corre y pbr.frag vuelve al camino
    // inline (samplear el volumen por pixel). El camino inline sigue existiendo
    // por eso, y es tambien el motivo por el que la presion de registros de
    // pbr.frag no baja con este cambio: el compilador reserva para la rama peor
    // aunque el flag del UBO sea uniforme. Sacarlo del todo pide un sistema de
    // permutaciones de shader, que el motor no tiene.
    //
    // Create devuelve nullopt si falta el shader (calcado de AoPass y DdgiPass):
    // sin IndirectPass, Application pasa un IndirectContext vacio y pbr.frag
    // samplea inline. Un fallo de carga no rompe el frame ni cambia la imagen.
    class IndirectPass {
        public:
            // Cuantos texels de resolucion completa cubre uno del target por eje.
            //
            // 2 y no 4: a un cuarto por eje (1/16 de los pixeles) el upsample se
            // queda sin taps validos demasiado seguido -- un objeto de menos de
            // 4 px de ancho no tiene NINGUN texel propio en el target reducido, y
            // el fallback al tap mas parecido deja de ser un escalon de un pixel
            // para volverse el color de otra superficie.
            static constexpr i32 kScale = 2;

            // De donde saca el bent normal ddgi/indirect.frag. Tienen que
            // coincidir con las constantes kBent* del shader: el target del AO
            // puede estar a resolucion completa o compartir la de este pase, y
            // leerlo en la grilla equivocada devuelve el bent normal de otro
            // pixel -- tuerce la direccion del color bleeding sin romper nada
            // ruidosamente, que es la peor clase de bug.
            static constexpr i32 kBentNinguno  = 0;
            static constexpr i32 kBentFullRes  = 1;
            static constexpr i32 kBentReducido = 2;

            static std::optional<IndirectPass> Create(Renderer& renderer, VertexArray& fullscreenQuad,
                                                      Shader* shader, u32 width, u32 height);

            IndirectPass(const IndirectPass&)            = delete;
            IndirectPass& operator=(const IndirectPass&) = delete;
            IndirectPass(IndirectPass&& other) noexcept;
            IndirectPass& operator=(IndirectPass&& other) noexcept;

            // depthNormal es el prepass del AO (FULL res) y ao su target: los dos
            // salen de AoPass. view/projection son las de la camara del frame;
            // se pasan y no se leen del bloque Frame porque la inversa de la view
            // hay que calcularla en CPU igual.
            //
            // No hace nada si el AO no dio un prepass utilizable: ver la nota de
            // dependencia arriba.
            void Render(const AoContext& ao, const Texture* depthNormal,
                        const Texture* aoTexture,
                        const glm::mat4& view, const glm::mat4& projection);

            void Resize(u32 fullWidth, u32 fullHeight);

            // Vacio (y por lo tanto invalido) hasta que Render haya corrido este
            // frame: pbr.frag tiene que caer al camino inline, no leer el
            // resultado del frame anterior con la camara de este.
            IndirectContext Context() const;

            bool enabled() const  { return m_enabled; }
            void SetEnabled(bool v) { m_enabled = v; }

            const Texture& target() const { return m_fb.ColorTexture(); }
            u32 width()  const { return m_fb.width(); }
            u32 height() const { return m_fb.height(); }

        private:
            IndirectPass(Renderer& renderer, VertexArray& fullscreenQuad, Shader* shader,
                         u32 width, u32 height);

            // Media resolucion redondeando HACIA ARRIBA: con 1079 de alto, un
            // floor dejaria la ultima fila de pixeles sin ningun texel que la
            // cubra y el upsample la resolveria con el clamp del borde.
            static u32 Reduced(u32 full) { return (full + static_cast<u32>(kScale) - 1u)
                                                / static_cast<u32>(kScale); }

            Renderer&    m_renderer;
            VertexArray& m_quad;
            Shader*      m_shader = null;

            Framebuffer m_fb;          // rgb = irradiancia indirecta, a = fade del volumen
            glm::ivec2  m_fullSize { 0, 0 };

            UniformBuffer m_ubo { sizeof(IndirectPassBlock) };

            bool m_enabled  = true;
            bool m_ranEste  = false;   // corrio en ESTE frame; lo resetea Render
            const Texture* m_depthNormal = null;
            bool m_aoReduced = false;
    };

}
}
