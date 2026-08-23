// Valida la maquina que ordena los pases del frame: orden de ejecucion, salteo
// de deshabilitados, propagacion de Resize y busqueda por tipo. Los pases son
// falsos y solo anotan su nombre: cero contexto OpenGL.
#include <doctest/doctest.h>
#include <efengine/renderer/ScenePipeline.h>

#include <memory>
#include <string>
#include <vector>

using efengine::renderer::FrameContext;
using efengine::renderer::IScenePass;
using efengine::renderer::ScenePipeline;

namespace {

    // Anota su nombre en el log compartido cada vez que corre. El log vive
    // afuera para que el test lo lea despues de que el pipeline sea dueno de
    // los pases.
    class FakePass : public IScenePass {
        public:
            FakePass(const char* name, std::vector<std::string>& log)
                : m_name(name), m_log(log) {}

            void Execute(FrameContext&) override { m_log.push_back(m_name); }

            void Resize(u32 w, u32 h) override {
                resizes++;
                lastWidth  = w;
                lastHeight = h;
            }

            const char* Name() const override { return m_name; }

            u32 resizes    = 0u;
            u32 lastWidth  = 0u;
            u32 lastHeight = 0u;

        private:
            const char*               m_name;
            std::vector<std::string>& m_log;
    };

    // Un segundo tipo, para que Find<T> tenga de donde elegir.
    class OtroPass : public FakePass {
        public:
            explicit OtroPass(std::vector<std::string>& log) : FakePass("Otro", log) {}
    };

}

TEST_CASE("ScenePipeline: ejecuta en el orden de registro") {
    std::vector<std::string> log;
    ScenePipeline pipeline;
    pipeline.Add(std::make_unique<FakePass>("A", log));
    pipeline.Add(std::make_unique<FakePass>("B", log));
    pipeline.Add(std::make_unique<FakePass>("C", log));

    FrameContext* ctx = nullptr;
    pipeline.Execute(ctx);

    REQUIRE(log.size() == 3u);
    CHECK(log[0] == "A");
    CHECK(log[1] == "B");
    CHECK(log[2] == "C");
}

TEST_CASE("ScenePipeline: un pase con enabled == false no se ejecuta") {
    std::vector<std::string> log;
    ScenePipeline pipeline;
    pipeline.Add(std::make_unique<FakePass>("A", log));
    IScenePass* b = pipeline.Add(std::make_unique<FakePass>("B", log));
    pipeline.Add(std::make_unique<FakePass>("C", log));

    b->enabled = false;

    FrameContext* ctx = nullptr;
    pipeline.Execute(ctx);

    REQUIRE(log.size() == 2u);
    CHECK(log[0] == "A");
    CHECK(log[1] == "C");
}

TEST_CASE("ScenePipeline: un pase deshabilitado SI recibe Resize") {
    // Si no lo recibiera, re-habilitarlo despues de un resize lo dejaria con
    // sus targets al tamano viejo.
    std::vector<std::string> log;
    ScenePipeline pipeline;
    FakePass* a = static_cast<FakePass*>(pipeline.Add(std::make_unique<FakePass>("A", log)));
    FakePass* b = static_cast<FakePass*>(pipeline.Add(std::make_unique<FakePass>("B", log)));
    b->enabled = false;

    pipeline.Resize(1280u, 720u);

    CHECK(a->resizes == 1u);
    CHECK(b->resizes == 1u);
    CHECK(b->lastWidth  == 1280u);
    CHECK(b->lastHeight == 720u);
}

TEST_CASE("ScenePipeline: Add devuelve un observador y Add(nullptr) no registra") {
    // Add(nullptr) es el caso 'Create fallo': el pase simplemente no esta en la
    // lista, y eso reemplaza a los std::optional de Application.
    std::vector<std::string> log;
    ScenePipeline pipeline;

    IScenePass* ok = pipeline.Add(std::make_unique<FakePass>("A", log));
    CHECK(ok != nullptr);
    CHECK(pipeline.PassCount() == 1u);

    IScenePass* fallo = pipeline.Add(nullptr);
    CHECK(fallo == nullptr);
    CHECK(pipeline.PassCount() == 1u);

    FrameContext* ctx = nullptr;
    pipeline.Execute(ctx);
    CHECK(log.size() == 1u);
}

TEST_CASE("ScenePipeline: Find encuentra por tipo dinamico y da null si no esta") {
    std::vector<std::string> log;
    ScenePipeline pipeline;
    pipeline.Add(std::make_unique<FakePass>("A", log));
    IScenePass* otro = pipeline.Add(std::make_unique<OtroPass>(log));

    CHECK(pipeline.Find<OtroPass>() == otro);
    CHECK(pipeline.Find<OtroPass>() != nullptr);

    ScenePipeline vacio;
    CHECK(vacio.Find<OtroPass>() == nullptr);
}
