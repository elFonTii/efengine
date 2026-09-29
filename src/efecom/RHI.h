// RHI.h
//  Render Hardware Interface de efecom: la única superficie por la que el
//  motor habla con la GPU. Hoy la implementa un backend OpenGL 4.5
//  (RHIOpenGL.cpp); mañana un backend Vulkan puede implementar estas mismas
//  funciones sin tocar el módulo renderer.
//
//  Reglas:
//   - Ningún tipo/enum/handle de una API gráfica concreta se asoma por acá.
//   - Los handles son u32 opacos (0 = handle inválido / "ninguno").
//   - efecom no depende de efengine: usa sus propios aliases (efecom/Types.h).
#pragma once

#include <efecom/Types.h>

namespace efecom {

    // ── Inicialización / contexto ───────────────────────────────────────────
    // El windowing (GLFW, SDL, ...) es responsabilidad del motor; el RHI solo
    // recibe el loader de funciones de la API una vez que el contexto está
    // current. Firma compatible con glfwGetProcAddress.
    using ProcFn            = void (*)();
    using ProcAddressLoader = ProcFn (*)(const char* name);

    // Carga la API gráfica. Devuelve false si falla (recuperable en el caller).
    bool Initialize(ProcAddressLoader loader);

    // Strings informativos del dispositivo/driver (válidos mientras viva el contexto).
    struct DeviceInfo {
        const char* apiVersion;
        const char* renderer;
        const char* vendor;
        const char* shadingLanguageVersion;
    };
    DeviceInfo GetDeviceInfo();

    // ── Diagnostico ─────────────────────────────────────────────────────────
    // El backend emite mensajes del driver (glDebugMessageCallback) por aca.
    // efecom no puede incluir el Log de efengine, asi que el motor instala un
    // sink y los mensajes terminan en el mismo log que todo lo demas.
    //
    // Sin sink instalado los mensajes van a stderr, igual que EFCOM_ASSERT.
    enum class MessageSeverity { Info, Warning, Error };
    using MessageSink = void (*)(MessageSeverity severity, const char* message);

    void SetMessageSink(MessageSink sink);

    // ── Estado global del pipeline ──────────────────────────────────────────
    void SetViewport(u32 x, u32 y, u32 width, u32 height);
    void SetClearColor(f32 r, f32 g, f32 b, f32 a);

    enum class ClearMask : u32 {
        Color      = 1u << 0,
        Depth      = 1u << 1,
        ColorDepth = Color | Depth,
    };
    void Clear(ClearMask mask);

    // Limpia un framebuffer SIN bindearlo y sin tocar el viewport. Existe para
    // los clears fuera de banda (limpiar un atlas, inicializar un target): con
    // BindRenderTarget habria que restaurar el viewport a mano y nadie lo hace.
    //
    // Igual que Clear, fuerza las mascaras de escritura: glClearNamedFramebuffer*
    // esta sujeto a los writemasks y al scissor, no los ignora.
    void ClearFramebuffer(u32 framebuffer, ClearMask mask);

    // ── Estado de rasterizacion ────────────────────────────────────────────
    // Todo el estado que en Vulkan es inmutable dentro de un pipeline object va
    // junto en un struct. Cada pase declara el suyo entero: nadie "restaura" nada,
    // porque restaurar depende de saber cual era el default y eso se rompe en
    // silencio cuando el default cambia.
    enum class DepthFunc   { Never, Less, Equal, LessEqual, Greater, NotEqual, GreaterEqual, Always };
    enum class CullMode    { None, Back, Front };
    enum class FrontFace   { CounterClockwise, Clockwise };
    enum class BlendFactor { Zero, One, SrcAlpha, OneMinusSrcAlpha, DstAlpha, OneMinusDstAlpha,
                             SrcColor, OneMinusSrcColor, DstColor, OneMinusDstColor };
    enum class BlendOp     { Add, Subtract, ReverseSubtract, Min, Max };

    struct PipelineState {
        bool        depthTest   = true;
        bool        depthWrite  = true;
        DepthFunc   depthFunc   = DepthFunc::Less;
        CullMode    cullMode    = CullMode::Back;
        FrontFace   frontFace   = FrontFace::CounterClockwise;
        bool        blendEnable = false;
        BlendFactor srcColor    = BlendFactor::SrcAlpha;
        BlendFactor dstColor    = BlendFactor::OneMinusSrcAlpha;
        BlendFactor srcAlpha    = BlendFactor::One;
        BlendFactor dstAlpha    = BlendFactor::OneMinusSrcAlpha;
        BlendOp     colorOp     = BlendOp::Add;
        BlendOp     alphaOp     = BlendOp::Add;
        bool        colorWrite[4] = { true, true, true, true };
    };

    // Comparacion campo por campo. El backend la usa para emitir solo las llamadas
    // gl* de lo que cambio; olvidarse un campo aca es un bug de estado invisible.
    inline bool operator==(const PipelineState& a, const PipelineState& b) {
        return a.depthTest   == b.depthTest
            && a.depthWrite  == b.depthWrite
            && a.depthFunc   == b.depthFunc
            && a.cullMode    == b.cullMode
            && a.frontFace   == b.frontFace
            && a.blendEnable == b.blendEnable
            && a.srcColor    == b.srcColor
            && a.dstColor    == b.dstColor
            && a.srcAlpha    == b.srcAlpha
            && a.dstAlpha    == b.dstAlpha
            && a.colorOp     == b.colorOp
            && a.alphaOp     == b.alphaOp
            && a.colorWrite[0] == b.colorWrite[0]
            && a.colorWrite[1] == b.colorWrite[1]
            && a.colorWrite[2] == b.colorWrite[2]
            && a.colorWrite[3] == b.colorWrite[3];
    }
    inline bool operator!=(const PipelineState& a, const PipelineState& b) { return !(a == b); }

    void ApplyPipelineState(const PipelineState& state);

    // Invalida el cache del backend: la proxima ApplyPipelineState reemite TODO.
    // Obligatorio cuando algo externo toca el estado de GL a espaldas del RHI
    // (el backend de ImGui lo hace en cada Render).
    void ResetPipelineStateCache();

    // ── Contadores del frame ────────────────────────────────────────────────
    // Viven en el RHI y no en el Renderer a proposito: aca abajo NO HAY FORMA
    // de dibujar sin ser contado. Puesto un nivel mas arriba, cualquier camino
    // de dibujo nuevo se escaparia de la cuenta por olvido.
    struct FrameCounters {
        u32 drawCalls  = 0u;
        u32 triangles  = 0u;
        u32 dispatches = 0u;

        // Cuantas veces se PIDIO aplicar estado, y cuantas de esas no cambiaron
        // absolutamente nada. La segunda es evidencia directa para el ciclo de
        // densidad de draws: Renderer::Submit reaplica estado por malla.
        u32 stateApplies   = 0u;
        u32 stateRedundant = 0u;
    };

    void          ResetFrameCounters();
    FrameCounters GetFrameCounters();

    // Solo el contador de draws. Existe aparte porque el profiler lo lee dos
    // veces por scope y no necesita copiar el struct entero.
    u32 GetDrawCallCount();

    // ── Buffers (VBO / EBO) ────────────────────────────────────────────────
    // Buffer estático: crea y sube los datos de una vez. Sirve tanto para
    // vértices como para índices (el uso lo decide el vertex array).
    u32  CreateBuffer(const void* data, usize size);
    void DestroyBuffer(u32 buffer);

    // Buffer de uniforms de contenido dinamico (se reescribe por frame o por draw).
    // DestroyBuffer sirve para liberarlo, igual que un VBO.
    u32  CreateUniformBuffer(usize size);
    void UpdateBuffer(u32 buffer, const void* data, usize size, usize offset);

    // Engancha el buffer entero a un indice de binding de uniform block. El
    // binding es estado GLOBAL, no por programa: alcanza con hacerlo una vez.
    //
    // Camino de upgrade Vulkan-friendly cuando el volumen de draws lo pida:
    // BindUniformBufferRange(buffer, binding, offset, size) sobre un ring buffer
    // de un frame, con offsets dinamicos. Se agrega aca sin tocar ni un shader.
    void BindUniformBuffer(u32 buffer, u32 bindingIndex);

    // Buffer de almacenamiento (SSBO). Igual que el de uniforms pero sin el
    // techo de 64 KB y indexable por gl_InstanceID desde el shader: es lo que
    // permite que UN draw instanciado dibuje N vistas distintas, cada una con su
    // matriz. DestroyBuffer sirve para liberarlo.
    u32  CreateStorageBuffer(usize size);
    void BindStorageBuffer(u32 buffer, u32 bindingIndex);

    // ── Vertex arrays ──────────────────────────────────────────────────────
    // Modelo de binding points (estilo DSA): el buffer se engancha a un
    // bindingIndex del VA y cada atributo (siempre floats) se asocia a él.
    u32  CreateVertexArray();
    void DestroyVertexArray(u32 va);
    void VertexArraySetVertexBuffer(u32 va, u32 bindingIndex, u32 buffer, u32 strideBytes);
    void VertexArraySetAttribute(u32 va, u32 location, u32 componentCount, u32 offsetBytes, u32 bindingIndex);
    void VertexArraySetIndexBuffer(u32 va, u32 indexBuffer);
    void BindVertexArray(u32 va);   // 0 = desbindear

    // ── Shaders / programas ────────────────────────────────────────────────
    enum class ShaderStage { Vertex, Fragment, Compute };

    // Compila un stage. Devuelve 0 si falla y deja el infolog en outLog
    // (truncado a logSize). outLog puede ser null si no interesa el log.
    u32  CompileShader(ShaderStage stage, const char* source, char* outLog, usize logSize);
    void DestroyShader(u32 shader);

    // Linkea un programa con 1 o 2 stages (stageB = 0 si no hay, p. ej.
    // compute). Devuelve 0 si falla y deja el infolog en outLog. Los stages
    // siguen siendo del caller: destruirlos después de linkear.
    u32  LinkProgram(u32 stageA, u32 stageB, char* outLog, usize logSize);
    void DestroyProgram(u32 program);
    void BindProgram(u32 program);  // 0 = desbindear

    // Uniforms: operan sobre el programa actualmente bindeado.

    // Texturas 2D
    // El formato interno determina también el layout de los pixeles de origen:
    // los formatos de 8 bits suben bytes, los flotantes/depth suben floats.
    enum class TextureFormat {
        R8, RG8, RGB8, RGBA8,   // lineales, 8 bits por canal
        SRGB8, SRGB8_A8,        // sRGB, 8 bits por canal
        RGBA16F, RG16F,         // HDR half-float (RG16F: la BRDF LUT, que solo usa 2 canales)
        Depth24, Depth32F,      // solo profundidad (attachments)
    };
    enum class TextureFilter { Nearest, Linear, LinearMipmapLinear };
    enum class TextureWrap   { Repeat, ClampToEdge, ClampToBorder };

    struct Texture2DDesc {
        u32           width     = 0;
        u32           height    = 0;
        TextureFormat format    = TextureFormat::RGBA8;
        TextureFilter minFilter = TextureFilter::Linear;
        TextureFilter magFilter = TextureFilter::Linear;
        TextureWrap   wrapS     = TextureWrap::Repeat;
        TextureWrap   wrapT     = TextureWrap::Repeat;
        bool          generateMipmaps = false;
        f32           borderColor[4]  = { 1.0f, 1.0f, 1.0f, 1.0f }; // para ClampToBorder
        // 1.0 = sin filtrado anisotropico (el default de GL). Se clampea contra
        // el maximo del device antes de llegar al driver.
        f32           maxAnisotropy   = 1.0f;
    };

    // Maximo de anisotropia que soporta el device (>= 1.0). Valido tras Initialize.
    f32 GetMaxAnisotropy();

    // Puro: acota lo pedido al rango que acepta el driver. Pedir de mas es un
    // GL_INVALID_VALUE, no un degradado silencioso.
    f32 ClampAnisotropy(f32 requested, f32 deviceMax);

    // pixels puede ser null (texturas vacías para attachments).
    u32  CreateTexture2D(const Texture2DDesc& desc, const void* pixels);
    void DestroyTexture(u32 texture);            // también libera cubemaps
    void BindTexture2D(u32 texture, u32 unit);   // unidad de textura para samplers

    struct Texture2DStorageDesc {
        u32           width     = 0;
        u32           height    = 0;
        TextureFormat format    = TextureFormat::RGBA16F;
        u32           mipCount  = 1;
        TextureFilter minFilter = TextureFilter::Linear;
        TextureFilter magFilter = TextureFilter::Linear;
        TextureWrap   wrapS     = TextureWrap::ClampToEdge;
        TextureWrap   wrapT     = TextureWrap::ClampToEdge;
    };
    u32 CreateTexture2DStorage(const Texture2DStorageDesc& desc);

    // Volumen de storage inmutable, sin mips. Para el grid de voxeles de DDGI.
    //
    // Filtro NEAREST: el trazado quiere el contenido del voxel que piso, no una
    // mezcla con sus vecinos. Un filtro lineal sobre la opacidad convertiria
    // cada superficie en una rampa de medio voxel y el DDA pegaria antes de
    // llegar a la geometria.
    struct Texture3DStorageDesc {
        u32           width  = 0;
        u32           height = 0;
        u32           depth  = 0;
        TextureFormat format = TextureFormat::RGBA8;
    };
    u32 CreateTexture3DStorage(const Texture3DStorageDesc& desc);

    // Deja el contenido de la textura en cero, sin subir un buffer desde CPU.
    void ClearTexture(u32 texture);

    // Array de 'layers' capas cuadradas de profundidad, storage inmutable, sin
    // mips. Para shadow maps en cascada: una capa por cascada, un solo sampler.
    //
    // Filtro NEAREST y borde blanco, igual que Texture::CreateDepthAttachment: el
    // PCF lo hace el fragment shader, y fuera del frustum la profundidad de borde
    // 1.0 significa "iluminado".
    u32 CreateDepthTexture2DArray(u32 resolution, u32 layers);

    // Cubemaps
    // Storage inmutable de 6 caras cuadradas con mipCount niveles.
    u32  CreateCubemap(u32 size, TextureFormat format, u32 mipCount);
    void BindTextureUnit(u32 texture, u32 unit); // bind directo (cualquier tipo de textura)
    void GenerateTextureMipmaps(u32 texture);

    // Imagen de shader (imageStore en compute)
    enum class ImageAccess { ReadOnly, WriteOnly, ReadWrite };
    void BindImageLayered(u32 unit, u32 texture, u32 level, ImageAccess access, TextureFormat format);

    // Imagen de shader bindea UN nivel de una textura 2D.
    void BindImage2D(u32 unit, u32 texture, u32 level, ImageAccess access, TextureFormat format);

    // Compute
    void DispatchCompute(u32 groupsX, u32 groupsY, u32 groupsZ);

    enum class Barrier : u32 {
        ShaderImageAccess = 1u << 0, // escrituras vía imageStore
        TextureFetch      = 1u << 1, // lecturas vía sampler
        ShaderStorage     = 1u << 2, // escrituras a SSBO desde un compute
    };
    void IssueMemoryBarrier(Barrier bits);

    // Framebuffers
    u32  CreateFramebuffer();
    void DestroyFramebuffer(u32 framebuffer);
    u32  GetPresentTarget();
    void SetPresentExtent(u32 width, u32 height);
    void GetPresentExtent(u32& outWidth, u32& outHeight);
    void BindRenderTarget(u32 target, u32 width, u32 height);
    void FramebufferColorTexture(u32 framebuffer, u32 texture);
    void FramebufferDepthTexture(u32 framebuffer, u32 texture);

    // Adjunta UNA capa de un array como el depth del FBO. Se vuelve a llamar
    // antes de cada cascada: un solo FBO, una capa por vez.
    void FramebufferDepthTextureLayer(u32 framebuffer, u32 texture, u32 layer);
    void FramebufferDisableColor(u32 framebuffer); // FBO solo-profundidad (shadow maps)

    // Tamano por defecto de un FBO SIN NINGUN attachment. Sin esto ese FBO es
    // incompleto y no se puede dibujar en el; con esto el rasterizador tiene un
    // area de barrido propia, independiente de la ventana. Lo usa la
    // voxelizacion, que no produce pixeles: su unico efecto son los imageStore.
    void FramebufferDefaultSize(u32 framebuffer, u32 width, u32 height);
    bool FramebufferComplete(u32 framebuffer);
    // Copia 1:1 del color de un FBO al backbuffer (NEAREST). No bindea nada.
    void BlitColorToPresent(u32 srcFramebuffer, u32 width, u32 height);
    u32  CreateDepthRenderbuffer(u32 width, u32 height);
    void DestroyRenderbuffer(u32 renderbuffer);
    void FramebufferDepthRenderbuffer(u32 framebuffer, u32 renderbuffer);

    // ── Consultas de marca de tiempo ────────────────────────────────────────
    // Una consulta guarda UNA marca, no un intervalo: para medir un pase hacen
    // falta dos, una antes y otra despues, y el tiempo es la resta.
    //
    // Se eligieron marcas y no el cronometro por bloque (GL_TIME_ELAPSED)
    // porque ese ultimo NO SE PUEDE ANIDAR: solo puede haber uno activo a la
    // vez, y un scope adentro de otro tiraria error de GL dejando los numeros
    // mal EN SILENCIO. Dos marcas independientes se anidan sin problema.
    bool TimestampQueriesSupported();

    u32  CreateTimestampQuery();
    void DestroyTimestampQuery(u32 query);

    // Encola "sellame el reloj cuando llegues aca" en el stream de comandos. No
    // bloquea: cuando esta funcion vuelve, la GPU seguramente todavia no llego.
    void WriteTimestamp(u32 query);

    // Si el resultado ya esta. NO bloquea. Hay que preguntar esto ANTES de
    // TimestampNanos, siempre.
    bool TimestampAvailable(u32 query);

    // El valor sellado, en nanosegundos. Llamarla sin que Available haya dado
    // true DETIENE LA CPU hasta que la GPU termine: el medidor se convierte en
    // el cuello de botella y mide su propia interferencia.
    u64  TimestampNanos(u32 query);

    // Draw (triángulos; índices u32)
    void DrawIndexed(u32 indexCount);
    void DrawArrays(u32 vertexCount);

    // Los mismos, repetidos instanceCount veces. El shader distingue cada copia
    // por gl_InstanceID; los contadores del frame siguen sumando UN draw call,
    // que es lo que la CPU paga, y las primitivas de todas las instancias.
    void DrawIndexedInstanced(u32 indexCount, u32 instanceCount);
    void DrawArraysInstanced(u32 vertexCount, u32 instanceCount);

    // ── Planos de recorte definidos por el shader ──────────────────────────
    // Habilita los primeros `count` gl_ClipDistance (maximo 8 por spec). Con
    // esto un draw instanciado puede recortar cada instancia a su propio
    // rectangulo del render target: es lo que hace posible dibujar N vistas en
    // un atlas 2D con un solo draw, sin un viewport por vista.
    //
    // OJO: mientras esten habilitados, TODO vertex shader que dibuje tiene que
    // escribir esas distancias. Un shader que no las escribe recorta contra
    // basura -- el resultado es indefinido, no "sin recortar".
    void SetClipDistanceCount(u32 count);   // 0 = deshabilitar todos

    // Operadores de bits para las máscaras
    inline constexpr ClearMask operator|(ClearMask a, ClearMask b) {
        return static_cast<ClearMask>(static_cast<u32>(a) | static_cast<u32>(b));
    }
    inline constexpr Barrier operator|(Barrier a, Barrier b) {
        return static_cast<Barrier>(static_cast<u32>(a) | static_cast<u32>(b));
    }

}
