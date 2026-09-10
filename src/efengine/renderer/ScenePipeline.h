#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/IScenePass.h>

#include <memory>
#include <typeinfo>
#include <vector>

namespace efengine {
namespace renderer {

    struct FrameContext;

    // Dueno de los pases del frame; los corre en el orden en que se
    // registraron. El orden ES la lista, igual que en PostChain: este tipo no
    // verifica dependencias entre pases -- las restricciones reales (que DDGI
    // corra despues del ShadowPass, que el prepass del AO corra antes del
    // forward) estan documentadas en cada pase y las garantiza quien arma la
    // lista.
    class ScenePipeline {
        public:
            // Devuelve un observador al pase registrado, o null si pass venia
            // nulo -- el caso "Create fallo". Mismo contrato que
            // ResourceManager (regla 8): null senala fallo, sin excepciones.
            // Un pase que no se pudo crear simplemente no esta en la lista, y
            // eso reemplaza a los std::optional que tenia Application.
            IScenePass* Add(std::unique_ptr<IScenePass> pass);

            // typeid sobre una referencia a tipo polimorfico da el tipo
            // DINAMICO real: el mismo mecanismo que BehaviorRegistry usa para
            // identificar behaviors al serializar. No es const porque los
            // paneles mutan los settings del pase que encuentran.
            template<class T>
            T* Find() {
                for (const std::unique_ptr<IScenePass>& p : m_passes) {
                    if (typeid(*p) == typeid(T)) return static_cast<T*>(p.get());
                }
                return null;
            }

            // ctx es puntero y no referencia porque los tests corren pases
            // falsos sin un FrameContext valido: sus referencias a SceneGraph,
            // Camera, Renderer y Framebuffer no son construibles sin GL.
            void Execute(FrameContext* ctx);

            // Va a TODOS los pases, incluidos los deshabilitados: si no,
            // re-habilitar uno despues de un resize lo dejaria con sus targets
            // al tamano viejo.
            void Resize(u32 width, u32 height);

            u32 PassCount() const { return static_cast<u32>(m_passes.size()); }

        private:
            std::vector<std::unique_ptr<IScenePass>> m_passes;
    };

}
}
