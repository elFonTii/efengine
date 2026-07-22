#pragma once
#include <efengine/renderer/IPostPass.h>
#include <efengine/renderer/Framebuffer.h>
#include <efengine/core/Types.h>
#include <vector>

// inline sirve para funciones pequeñas y recurrentes
// le dice al comp que inserte el código de la funcion en los lugares que se llama.

namespace efengine {
namespace renderer {
    struct PassRoute { bool toBackbuffer; u32 scratch; };
    inline PassRoute postChainTarget(u32 index, u32 count) {
        return { index + 1 == count, index & 1u};
    }

    class PostChain {
        public:
            PostChain(u32 width, u32 height);

            void Add(IPostPass*);
            void Run(const Texture&);
            void Resize(u32 width, u32 height);
    
        private:
            std::vector<IPostPass*> m_passes; // guarda los pases de postprocesado
            Framebuffer scratch_1;
            Framebuffer scratch_2;
    };
}
}

