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

    // ── Estado global del pipeline ──────────────────────────────────────────
    void SetViewport(u32 x, u32 y, u32 width, u32 height);
    void SetClearColor(f32 r, f32 g, f32 b, f32 a);

    enum class ClearMask : u32 {
        Color      = 1u << 0,
        Depth      = 1u << 1,
        ColorDepth = Color | Depth,
    };
    void Clear(ClearMask mask);

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
    // GetUniformLocation devuelve -1 si el uniform no existe (fue optimizado).
    i32  GetUniformLocation(u32 program, const char* name);
    void SetUniformInt(i32 location, i32 value);
    void SetUniformFloat(i32 location, f32 value);
    void SetUniformVec3(i32 location, const f32* values3);   // 3 floats
    void SetUniformMat4(i32 location, const f32* values16);  // 16 floats, column-major

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
    };

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
    void FramebufferDisableColor(u32 framebuffer); // FBO solo-profundidad (shadow maps)
    bool FramebufferComplete(u32 framebuffer);
    u32  CreateDepthRenderbuffer(u32 width, u32 height);
    void DestroyRenderbuffer(u32 renderbuffer);
    void FramebufferDepthRenderbuffer(u32 framebuffer, u32 renderbuffer);

    // Draw (triángulos; índices u32)
    void DrawIndexed(u32 indexCount);
    void DrawArrays(u32 vertexCount);

    // Operadores de bits para las máscaras
    inline constexpr ClearMask operator|(ClearMask a, ClearMask b) {
        return static_cast<ClearMask>(static_cast<u32>(a) | static_cast<u32>(b));
    }
    inline constexpr Barrier operator|(Barrier a, Barrier b) {
        return static_cast<Barrier>(static_cast<u32>(a) | static_cast<u32>(b));
    }

}
