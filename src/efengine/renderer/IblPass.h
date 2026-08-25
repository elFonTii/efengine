#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/Environment.h>
#include <efengine/renderer/IScenePass.h>

#include <memory>
#include <optional>

namespace efengine {
namespace renderer {

    // Dueno del Environment precomputado. No dibuja nada: publica las piezas
    // del IBL en el contexto del frame.
    //
    // Existe como pase y no como un miembro suelto de Application para que el
    // IBL entre en la misma lista que todo lo demas, y para que su ausencia se
    // exprese igual que la de cualquier otro pase: no estando en la lista.
    class IblPass : public IScenePass {
        public:
            // null si el Environment no se pudo crear. Sin este pase, los
            // punteros del IblContext quedan en null y el shader apaga el
            // ambiente entero (uHasIbl = 0) en vez de muestrear una unidad de
            // textura equivocada.
            static std::unique_ptr<IblPass> Create(std::optional<Environment>&& env);

            void Execute(FrameContext& ctx) override;

            const char* Name() const override { return "IBL"; }

            const Environment& environment() const { return m_env; }

        private:
            explicit IblPass(Environment&& env) : m_env(std::move(env)) {}

            Environment m_env;
    };

}
}
