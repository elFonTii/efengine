#include "efengine/renderer/PostChain.h"
#include <efengine/core/Assert.h>
#include <glad/gl.h>

namespace efengine {
namespace renderer {
    PostChain::PostChain(u32 width, u32 height) 
        : scratch_1(width, height),
          scratch_2(width, height) {}

    void PostChain::Add(IPostPass* pass) {
        m_passes.push_back(pass);
    }

    void PostChain::Resize(u32 width, u32 height) {
        scratch_1.Resize(width, height);
        scratch_2.Resize(width, height);

        for (IPostPass* p : m_passes) p->Resize(width, height); // redimensiona todos los pases de postprocesado
    }

    void PostChain::Run(const Texture& sceneHDR) {
        glDisable(GL_DEPTH_TEST);
        const u32 count = static_cast<u32>(m_passes.size());
        const Texture* input = &sceneHDR;

        for(u32 i = 0; i < count; i++) {
            PassRoute route = postChainTarget(i, count);

            Framebuffer* target = route.toBackbuffer
                ? null
                : (route.scratch == 0 ? &scratch_1 : &scratch_2);

            m_passes[i]->Apply(*input, target);

            if(!route.toBackbuffer)
                input = &target->ColorTexture();
        }
        
        glEnable(GL_DEPTH_TEST);
    }
}
}