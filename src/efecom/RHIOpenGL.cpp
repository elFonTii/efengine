// RHIOpenGL.cpp
//  Backend OpenGL 4.5 del RHI. Este es el ÚNICO translation unit del proyecto
//  que incluye glad: ninguna llamada gl* ni enum GL_* puede existir fuera de
//  acá. Un backend Vulkan futuro sería otro .cpp implementando RHI.h.
#include "RHI.h"
#include "Assert.h"

#include <glad/gl.h>

#include <cstdio>

namespace efecom {

    // ── Mapeos de enums RHI → GL ───────────────────────────────────────────
    namespace {

        struct GLFormat {
            GLenum internalFormat; // cómo lo guarda la GPU
            GLenum pixelFormat;    // layout de canales del origen
            GLenum pixelType;      // tipo de cada canal del origen
        };

        GLFormat to_gl(TextureFormat format) {
            switch (format) {
                case TextureFormat::R8:       return { GL_R8,                  GL_RED,             GL_UNSIGNED_BYTE };
                case TextureFormat::RG8:      return { GL_RG8,                 GL_RG,              GL_UNSIGNED_BYTE };
                case TextureFormat::RGB8:     return { GL_RGB8,                GL_RGB,             GL_UNSIGNED_BYTE };
                case TextureFormat::RGBA8:    return { GL_RGBA8,               GL_RGBA,            GL_UNSIGNED_BYTE };
                case TextureFormat::SRGB8:    return { GL_SRGB8,               GL_RGB,             GL_UNSIGNED_BYTE };
                case TextureFormat::SRGB8_A8: return { GL_SRGB8_ALPHA8,        GL_RGBA,            GL_UNSIGNED_BYTE };
                case TextureFormat::RGBA16F:  return { GL_RGBA16F,             GL_RGBA,            GL_FLOAT };
                case TextureFormat::RG16F:    return { GL_RG16F,               GL_RG,              GL_FLOAT };
                case TextureFormat::Depth24:  return { GL_DEPTH_COMPONENT24,   GL_DEPTH_COMPONENT, GL_FLOAT };
                case TextureFormat::Depth32F: return { GL_DEPTH_COMPONENT32F,  GL_DEPTH_COMPONENT, GL_FLOAT };
            }
            EFCOM_ASSERT(false, "to_gl: TextureFormat desconocido");
            return { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };
        }

        GLint to_gl(TextureFilter filter) {
            switch (filter) {
                case TextureFilter::Nearest:            return GL_NEAREST;
                case TextureFilter::Linear:             return GL_LINEAR;
                case TextureFilter::LinearMipmapLinear: return GL_LINEAR_MIPMAP_LINEAR;
            }
            EFCOM_ASSERT(false, "to_gl: TextureFilter desconocido");
            return GL_LINEAR;
        }

        GLint to_gl(TextureWrap wrap) {
            switch (wrap) {
                case TextureWrap::Repeat:        return GL_REPEAT;
                case TextureWrap::ClampToEdge:   return GL_CLAMP_TO_EDGE;
                case TextureWrap::ClampToBorder: return GL_CLAMP_TO_BORDER;
            }
            EFCOM_ASSERT(false, "to_gl: TextureWrap desconocido");
            return GL_REPEAT;
        }

        GLenum to_gl(ShaderStage stage) {
            switch (stage) {
                case ShaderStage::Vertex:   return GL_VERTEX_SHADER;
                case ShaderStage::Fragment: return GL_FRAGMENT_SHADER;
                case ShaderStage::Compute:  return GL_COMPUTE_SHADER;
            }
            EFCOM_ASSERT(false, "to_gl: ShaderStage desconocido");
            return GL_VERTEX_SHADER;
        }

        GLenum to_gl(ImageAccess access) {
            switch (access) {
                case ImageAccess::ReadOnly:  return GL_READ_ONLY;
                case ImageAccess::WriteOnly: return GL_WRITE_ONLY;
                case ImageAccess::ReadWrite: return GL_READ_WRITE;
            }
            EFCOM_ASSERT(false, "to_gl: ImageAccess desconocido");
            return GL_READ_ONLY;
        }

    }

    // ── Diagnostico ────────────────────────────────────────────────────────
    namespace {
        MessageSink g_messageSink = nullptr;

        void emit(MessageSeverity sev, const char* msg) {
            if (g_messageSink != nullptr) { g_messageSink(sev, msg); return; }
            std::fprintf(stderr, "[EFCOM GL] %s\n", msg);
        }

        // GLAD_API_PTR y no APIENTRY: es el spelling que define el propio glad,
        // y APIENTRY solo existe si antes se incluyo windows.h.
        void GLAD_API_PTR debugCallback(GLenum /*source*/, GLenum type, GLuint /*id*/,
                                        GLenum severity, GLsizei /*length*/,
                                        const GLchar* message, const void* /*user*/) {
            MessageSeverity sev = MessageSeverity::Info;
            if (severity == GL_DEBUG_SEVERITY_HIGH)        sev = MessageSeverity::Error;
            else if (severity == GL_DEBUG_SEVERITY_MEDIUM) sev = MessageSeverity::Warning;
            else if (type == GL_DEBUG_TYPE_ERROR)          sev = MessageSeverity::Error;
            emit(sev, message);
        }
    }

    void SetMessageSink(MessageSink sink) { g_messageSink = sink; }

    // ── Inicialización / contexto ───────────────────────────────────────────
    bool Initialize(ProcAddressLoader loader) {
        EFCOM_ASSERT(loader != nullptr, "Initialize: loader no puede ser null");
        if (gladLoadGL((GLADloadfunc)loader) == 0) return false;

#ifdef _DEBUG
        // SYNCHRONOUS es lo que hace util al callback: sin el dispara en
        // cualquier momento posterior y el stack no dice nada. Con el, un
        // breakpoint aca para en la llamada gl* culpable. Cuesta rendimiento,
        // por eso solo en debug.
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(debugCallback, nullptr);

        // El driver de NVIDIA emite una NOTIFICATION por cada asignacion de
        // buffer. Sin este filtro el log queda ilegible y se deja de leer, que
        // es exactamente el problema que este callback viene a resolver.
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION,
                              0, nullptr, GL_FALSE);
#endif
        return true;
    }

    DeviceInfo GetDeviceInfo() {
        // glGetString devuelve const GLubyte*; casteamos a const char*.
        DeviceInfo info;
        info.apiVersion             = (const char*)glGetString(GL_VERSION);
        info.renderer               = (const char*)glGetString(GL_RENDERER);
        info.vendor                 = (const char*)glGetString(GL_VENDOR);
        info.shadingLanguageVersion = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
        return info;
    }

    // ── Estado global del pipeline ──────────────────────────────────────────
    void SetViewport(u32 x, u32 y, u32 width, u32 height) {
        glViewport((GLint)x, (GLint)y, (GLsizei)width, (GLsizei)height);
    }

    void SetClearColor(f32 r, f32 g, f32 b, f32 a) { glClearColor(r, g, b, a); }

    namespace {
        // Ultimo estado aplicado. g_stateValid = false fuerza reemitir todo:
        // arranca invalido porque el estado real de GL al abrir el contexto no
        // tiene por que coincidir con los defaults de PipelineState.
        //
        // Vive aca arriba, y no junto a ApplyPipelineState, porque Clear tambien
        // lo escribe: un clear cambia las mascaras y el cache tiene que enterarse.
        PipelineState g_state;
        bool          g_stateValid = false;

        FrameCounters g_counters;
    }

    void Clear(ClearMask mask) {
        GLbitfield bits = 0;
        if ((u32)mask & (u32)ClearMask::Color) bits |= GL_COLOR_BUFFER_BIT;
        if ((u32)mask & (u32)ClearMask::Depth) bits |= GL_DEPTH_BUFFER_BIT;
        if (bits == 0) return;

        // glClear obedece las mascaras de escritura y el scissor. Sin forzarlos,
        // que un Clear limpie o no depende de que pase aplico estado antes: con
        // las sombras apagadas nadie aplica nada, el depthMask sigue en FALSE
        // desde el post del frame anterior, y el depth no se limpia.
        glDisable(GL_SCISSOR_TEST);
        if (bits & GL_DEPTH_BUFFER_BIT) glDepthMask(GL_TRUE);
        if (bits & GL_COLOR_BUFFER_BIT) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        glClear(bits);

        // El cache tiene que reflejar lo que quedo emitido. Sin esto, el proximo
        // ApplyPipelineState compara contra un g_state que miente y se saltea
        // llamadas que si hacen falta.
        if (bits & GL_DEPTH_BUFFER_BIT) g_state.depthWrite = true;
        if (bits & GL_COLOR_BUFFER_BIT) {
            g_state.colorWrite[0] = g_state.colorWrite[1] =
            g_state.colorWrite[2] = g_state.colorWrite[3] = true;
        }
    }

    // ── Estado de rasterizacion ────────────────────────────────────────────
    namespace {
        GLenum toGlDepthFunc(DepthFunc f) {
            switch (f) {
                case DepthFunc::Never:        return GL_NEVER;
                case DepthFunc::Less:         return GL_LESS;
                case DepthFunc::Equal:        return GL_EQUAL;
                case DepthFunc::LessEqual:    return GL_LEQUAL;
                case DepthFunc::Greater:      return GL_GREATER;
                case DepthFunc::NotEqual:     return GL_NOTEQUAL;
                case DepthFunc::GreaterEqual: return GL_GEQUAL;
                case DepthFunc::Always:       return GL_ALWAYS;
            }
            return GL_LESS;
        }

        GLenum toGlBlendFactor(BlendFactor f) {
            switch (f) {
                case BlendFactor::Zero:             return GL_ZERO;
                case BlendFactor::One:              return GL_ONE;
                case BlendFactor::SrcAlpha:         return GL_SRC_ALPHA;
                case BlendFactor::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
                case BlendFactor::DstAlpha:         return GL_DST_ALPHA;
                case BlendFactor::OneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
                case BlendFactor::SrcColor:         return GL_SRC_COLOR;
                case BlendFactor::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
                case BlendFactor::DstColor:         return GL_DST_COLOR;
                case BlendFactor::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
            }
            return GL_ONE;
        }

        GLenum toGlBlendOp(BlendOp op) {
            switch (op) {
                case BlendOp::Add:             return GL_FUNC_ADD;
                case BlendOp::Subtract:        return GL_FUNC_SUBTRACT;
                case BlendOp::ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
                case BlendOp::Min:             return GL_MIN;
                case BlendOp::Max:             return GL_MAX;
            }
            return GL_FUNC_ADD;
        }

    }

    void ResetPipelineStateCache() { g_stateValid = false; }

    void ResetFrameCounters() { g_counters = FrameCounters{}; }

    FrameCounters GetFrameCounters() { return g_counters; }

    u32 GetDrawCallCount() { return g_counters.drawCalls; }

    void ApplyPipelineState(const PipelineState& s) {
        ++g_counters.stateApplies;
        // El operator== de PipelineState ya existe (RHI.h). Si el cache es
        // valido y el estado pedido es identico al vigente, esta llamada no va a
        // emitir una sola instruccion de GL: es puro overhead de CPU.
        if (g_stateValid && s == g_state) ++g_counters.stateRedundant;

        const bool all = !g_stateValid;

        if (all || s.depthTest != g_state.depthTest) {
            if (s.depthTest) glEnable(GL_DEPTH_TEST);
            else             glDisable(GL_DEPTH_TEST);
        }
        if (all || s.depthWrite != g_state.depthWrite) {
            glDepthMask(s.depthWrite ? GL_TRUE : GL_FALSE);
        }
        if (all || s.depthFunc != g_state.depthFunc) {
            glDepthFunc(toGlDepthFunc(s.depthFunc));
        }
        if (all || s.cullMode != g_state.cullMode) {
            if (s.cullMode == CullMode::None) {
                glDisable(GL_CULL_FACE);
            } else {
                glEnable(GL_CULL_FACE);
                glCullFace(s.cullMode == CullMode::Back ? GL_BACK : GL_FRONT);
            }
        }
        if (all || s.frontFace != g_state.frontFace) {
            glFrontFace(s.frontFace == FrontFace::CounterClockwise ? GL_CCW : GL_CW);
        }
        if (all || s.blendEnable != g_state.blendEnable) {
            if (s.blendEnable) glEnable(GL_BLEND);
            else               glDisable(GL_BLEND);
        }
        if (all || s.srcColor != g_state.srcColor || s.dstColor != g_state.dstColor
                || s.srcAlpha != g_state.srcAlpha || s.dstAlpha != g_state.dstAlpha) {
            glBlendFuncSeparate(toGlBlendFactor(s.srcColor), toGlBlendFactor(s.dstColor),
                                toGlBlendFactor(s.srcAlpha), toGlBlendFactor(s.dstAlpha));
        }
        if (all || s.colorOp != g_state.colorOp || s.alphaOp != g_state.alphaOp) {
            glBlendEquationSeparate(toGlBlendOp(s.colorOp), toGlBlendOp(s.alphaOp));
        }
        if (all || s.colorWrite[0] != g_state.colorWrite[0]
                || s.colorWrite[1] != g_state.colorWrite[1]
                || s.colorWrite[2] != g_state.colorWrite[2]
                || s.colorWrite[3] != g_state.colorWrite[3]) {
            glColorMask(s.colorWrite[0] ? GL_TRUE : GL_FALSE,
                        s.colorWrite[1] ? GL_TRUE : GL_FALSE,
                        s.colorWrite[2] ? GL_TRUE : GL_FALSE,
                        s.colorWrite[3] ? GL_TRUE : GL_FALSE);
        }

        g_state      = s;
        g_stateValid = true;
    }

    // ── Buffers ────────────────────────────────────────────────────────────
    u32 CreateBuffer(const void* data, usize size) {
        u32 id = 0;
        glCreateBuffers(1, &id); // GL 4.5 DSA
        EFCOM_ASSERT(id != 0, "CreateBuffer: glCreateBuffers fallo (sin contexto GL)");
        glNamedBufferData(id, (GLsizeiptr)size, data, GL_STATIC_DRAW);
        return id;
    }

    void DestroyBuffer(u32 buffer) { glDeleteBuffers(1, &buffer); }

    u32 CreateUniformBuffer(usize size) {
        u32 id = 0;
        glCreateBuffers(1, &id);
        EFCOM_ASSERT(id != 0, "CreateUniformBuffer: glCreateBuffers fallo (sin contexto GL)");
        // DYNAMIC_DRAW: se reescribe por frame o por draw, no es contenido estatico.
        glNamedBufferData(id, (GLsizeiptr)size, nullptr, GL_DYNAMIC_DRAW);
        return id;
    }

    void UpdateBuffer(u32 buffer, const void* data, usize size, usize offset) {
        glNamedBufferSubData(buffer, (GLintptr)offset, (GLsizeiptr)size, data);
    }

    void BindUniformBuffer(u32 buffer, u32 bindingIndex) {
        glBindBufferBase(GL_UNIFORM_BUFFER, (GLuint)bindingIndex, (GLuint)buffer);
    }

    // ── Vertex arrays ──────────────────────────────────────────────────────
    u32 CreateVertexArray() {
        u32 id = 0;
        glCreateVertexArrays(1, &id);
        EFCOM_ASSERT(id != 0, "CreateVertexArray: glCreateVertexArrays fallo (sin contexto GL)");
        return id;
    }

    void DestroyVertexArray(u32 va) { glDeleteVertexArrays(1, &va); }

    void VertexArraySetVertexBuffer(u32 va, u32 bindingIndex, u32 buffer, u32 strideBytes) {
        glVertexArrayVertexBuffer(va, bindingIndex, buffer, 0, (GLsizei)strideBytes);
    }

    void VertexArraySetAttribute(u32 va, u32 location, u32 componentCount, u32 offsetBytes, u32 bindingIndex) {
        glEnableVertexArrayAttrib(va, location);
        glVertexArrayAttribFormat(va, location, (GLint)componentCount, GL_FLOAT, GL_FALSE, (GLuint)offsetBytes);
        glVertexArrayAttribBinding(va, location, bindingIndex);
    }

    void VertexArraySetIndexBuffer(u32 va, u32 indexBuffer) {
        glVertexArrayElementBuffer(va, indexBuffer);
    }

    void BindVertexArray(u32 va) { glBindVertexArray(va); }

    // ── Shaders / programas ────────────────────────────────────────────────
    u32 CompileShader(ShaderStage stage, const char* source, char* outLog, usize logSize) {
        EFCOM_ASSERT(source != nullptr, "CompileShader: source no puede ser null");

        const u32 shader = glCreateShader(to_gl(stage));
        EFCOM_ASSERT(shader != 0, "CompileShader: glCreateShader devolvio 0 (sin contexto GL)");
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint ok = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (ok != GL_TRUE) {
            if (outLog != nullptr && logSize > 0) {
                glGetShaderInfoLog(shader, (GLsizei)logSize, nullptr, outLog);
            }
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    void DestroyShader(u32 shader) { glDeleteShader(shader); }

    u32 LinkProgram(u32 stageA, u32 stageB, char* outLog, usize logSize) {
        const u32 program = glCreateProgram();
        EFCOM_ASSERT(program != 0, "LinkProgram: glCreateProgram devolvio 0 (sin contexto GL)");

        if (stageA != 0) glAttachShader(program, stageA);
        if (stageB != 0) glAttachShader(program, stageB);
        glLinkProgram(program);

        GLint ok = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &ok);
        if (ok != GL_TRUE) {
            if (outLog != nullptr && logSize > 0) {
                glGetProgramInfoLog(program, (GLsizei)logSize, nullptr, outLog);
            }
            glDeleteProgram(program);
            return 0;
        }
        return program;
    }

    void DestroyProgram(u32 program) { glDeleteProgram(program); }
    void BindProgram(u32 program)    { glUseProgram(program); }



    // ── Texturas 2D ────────────────────────────────────────────────────────
    u32 CreateTexture2D(const Texture2DDesc& desc, const void* pixels) {
        const GLFormat fmt = to_gl(desc.format);

        u32 id = 0;
        glCreateTextures(GL_TEXTURE_2D, 1, &id);
        EFCOM_ASSERT(id != 0, "CreateTexture2D: glCreateTextures fallo (sin contexto GL)");

        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, to_gl(desc.minFilter));
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, to_gl(desc.magFilter));
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, to_gl(desc.wrapS));
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, to_gl(desc.wrapT));
        if (desc.wrapS == TextureWrap::ClampToBorder || desc.wrapT == TextureWrap::ClampToBorder) {
            glTextureParameterfv(id, GL_TEXTURE_BORDER_COLOR, desc.borderColor);
        }

        // Storage inmutable: el tamano y el formato quedan fijos. Es lo correcto
        // para un attachment, y obliga a que Framebuffer::Resize recree la
        // textura entera en vez de re-especificarla (ya lo hace).
        //
        // mipCount 1 cuando no se piden mipmaps; si se piden, la cadena completa.
        u32 mips = 1u;
        if (desc.generateMipmaps) {
            u32 lado = desc.width > desc.height ? desc.width : desc.height;
            while (lado > 1u) { lado >>= 1; ++mips; }
        }
        glTextureStorage2D(id, (GLsizei)mips, fmt.internalFormat,
                           (GLsizei)desc.width, (GLsizei)desc.height);

        if (pixels != nullptr) {
            // El default de UNPACK_ALIGNMENT es 4. Una fila RGB8 de ancho no
            // multiplo de 4 no cae en esa alineacion (1023 * 3 = 3069, y
            // 3069 % 4 = 1), y cada fila arranca corrida: es el sesgo diagonal.
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTextureSubImage2D(id, 0, 0, 0, (GLsizei)desc.width, (GLsizei)desc.height,
                                fmt.pixelFormat, fmt.pixelType, pixels);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);   // lo dejamos como estaba
        }

        if (desc.generateMipmaps) {
            glGenerateTextureMipmap(id);
        }

        return id;
    }

    void DestroyTexture(u32 texture) { glDeleteTextures(1, &texture); }

    void BindTexture2D(u32 texture, u32 unit) {
        // glBindTextureUnit y no glActiveTexture + glBindTexture: la version
        // vieja dejaba la unidad activa cambiada como efecto de borde, y quien
        // llamara despues heredaba ese estado sin saberlo.
        glBindTextureUnit(unit, texture);
    }

    u32 CreateTexture2DStorage(const Texture2DStorageDesc& desc) {
        u32 id = 0;
        glCreateTextures(GL_TEXTURE_2D, 1, &id);
        EFCOM_ASSERT(id != 0, "CreateTexture2DStorage: glCreateTextures devuelve 0 (sin contexto GL)");
        
        glTextureStorage2D(id, (GLsizei)desc.mipCount, to_gl(desc.format).internalFormat,
                           (GLsizei)desc.width, (GLsizei)desc.height);

        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, to_gl(desc.minFilter));
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, to_gl(desc.magFilter));
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, to_gl(desc.wrapS));
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, to_gl(desc.wrapT));

        return id;
    }

    // ── Cubemaps ───────────────────────────────────────────────────────────
    u32 CreateCubemap(u32 size, TextureFormat format, u32 mipCount) {
        u32 id = 0;
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);
        EFCOM_ASSERT(id != 0, "CreateCubemap: glCreateTextures devuelve 0 (sin contexto GL)");
        // storage2d reserva las 6 caras del cubemap
        glTextureStorage2D(id, (GLsizei)mipCount, to_gl(format).internalFormat, (GLsizei)size, (GLsizei)size);

        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        return id;
    }

    void BindTextureUnit(u32 texture, u32 unit) { glBindTextureUnit(unit, texture); }
    void GenerateTextureMipmaps(u32 texture)    { glGenerateTextureMipmap(texture); }

    void BindImageLayered(u32 unit, u32 texture, u32 level, ImageAccess access, TextureFormat format) {
        glBindImageTexture(unit, texture, (GLint)level, GL_TRUE, 0, to_gl(access), to_gl(format).internalFormat);
    }

    void BindImage2D(u32 unit, u32 texture, u32 level, ImageAccess access, TextureFormat format) {
        glBindImageTexture(unit, texture, (GLint)level, GL_FALSE, 0, to_gl(access), to_gl(format).internalFormat);
    }

    // ── Compute ────────────────────────────────────────────────────────────
    void DispatchCompute(u32 groupsX, u32 groupsY, u32 groupsZ) {
        ++g_counters.dispatches;
        glDispatchCompute(groupsX, groupsY, groupsZ);
    }

    void IssueMemoryBarrier(Barrier bits) {
        GLbitfield glBits = 0;
        if ((u32)bits & (u32)Barrier::ShaderImageAccess) glBits |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
        if ((u32)bits & (u32)Barrier::TextureFetch)      glBits |= GL_TEXTURE_FETCH_BARRIER_BIT;
        glMemoryBarrier(glBits);
    }

    // ── Framebuffers ───────────────────────────────────────────────────────
    u32 CreateFramebuffer() {
        u32 id = 0;
        glCreateFramebuffers(1, &id); // GL 4.5 DSA
        EFCOM_ASSERT(id != 0, "CreateFramebuffer: glCreateFramebuffers fallo (sin contexto GL)");
        return id;
    }

    void DestroyFramebuffer(u32 framebuffer) { glDeleteFramebuffers(1, &framebuffer); }

    // Extent del backbuffer. Lo fija el motor en el init del contexto y en cada
    // resize de ventana; en un backend Vulkan saldria del swapchain.
    namespace { u32 g_presentWidth = 0u; u32 g_presentHeight = 0u; }

    u32  GetPresentTarget() { return 0u; }   // en GL el backbuffer ES el FBO 0
    void SetPresentExtent(u32 width, u32 height) {
        g_presentWidth  = width;
        g_presentHeight = height;
    }

    // Solo lo consume renderer::RenderTarget::Present(). No toca GL.
    void GetPresentExtent(u32& outWidth, u32& outHeight) {
        outWidth  = g_presentWidth;
        outHeight = g_presentHeight;
    }

    void BindRenderTarget(u32 target, u32 width, u32 height) {
        glBindFramebuffer(GL_FRAMEBUFFER, target);
        glViewport(0, 0, (GLsizei)width, (GLsizei)height);
    }

    void FramebufferColorTexture(u32 framebuffer, u32 texture) {
        glNamedFramebufferTexture(framebuffer, GL_COLOR_ATTACHMENT0, texture, 0);
    }

    void FramebufferDepthTexture(u32 framebuffer, u32 texture) {
        glNamedFramebufferTexture(framebuffer, GL_DEPTH_ATTACHMENT, texture, 0);
    }

    void FramebufferDisableColor(u32 framebuffer) {
        // Sin color attachment: no dibujamos ni leemos color.
        glNamedFramebufferDrawBuffer(framebuffer, GL_NONE);
        glNamedFramebufferReadBuffer(framebuffer, GL_NONE);
    }

    bool FramebufferComplete(u32 framebuffer) {
        return glCheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    }

    void ClearFramebuffer(u32 framebuffer, ClearMask mask) {
        glDisable(GL_SCISSOR_TEST);

        if ((u32)mask & (u32)ClearMask::Color) {
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            const GLfloat cero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            glClearNamedFramebufferfv(framebuffer, GL_COLOR, 0, cero);
            g_state.colorWrite[0] = g_state.colorWrite[1] =
            g_state.colorWrite[2] = g_state.colorWrite[3] = true;
        }
        if ((u32)mask & (u32)ClearMask::Depth) {
            glDepthMask(GL_TRUE);
            const GLfloat uno = 1.0f;
            glClearNamedFramebufferfv(framebuffer, GL_DEPTH, 0, &uno);
            g_state.depthWrite = true;
        }
    }

    u32 CreateDepthRenderbuffer(u32 width, u32 height) {
        u32 id = 0;
        glCreateRenderbuffers(1, &id);
        EFCOM_ASSERT(id != 0, "CreateDepthRenderbuffer: glCreateRenderbuffers fallo (sin contexto GL)");
        glNamedRenderbufferStorage(id, GL_DEPTH_COMPONENT24, (GLsizei)width, (GLsizei)height);
        return id;
    }

    void DestroyRenderbuffer(u32 renderbuffer) { glDeleteRenderbuffers(1, &renderbuffer); }

    void FramebufferDepthRenderbuffer(u32 framebuffer, u32 renderbuffer) {
        glNamedFramebufferRenderbuffer(framebuffer, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer);
    }

    // ── Consultas de marca de tiempo ───────────────────────────────────────
    bool TimestampQueriesSupported() {
        // GL_TIMESTAMP es core desde 3.3, pero un driver puede reportar cero
        // bits de contador, que significa "la implemento pero no la mide". Ese,
        // y no la version de GL, es el chequeo correcto.
        GLint bits = 0;
        glGetQueryiv(GL_TIMESTAMP, GL_QUERY_COUNTER_BITS, &bits);
        return bits > 0;
    }

    u32 CreateTimestampQuery() {
        u32 id = 0;
        glCreateQueries(GL_TIMESTAMP, 1, &id);   // GL 4.5 DSA, igual que el resto del backend
        EFCOM_ASSERT(id != 0, "CreateTimestampQuery: glCreateQueries fallo (sin contexto GL)");
        return id;
    }

    void DestroyTimestampQuery(u32 query) {
        if (query == 0) return;
        glDeleteQueries(1, &query);
    }

    void WriteTimestamp(u32 query) {
        if (query == 0) return;
        glQueryCounter(query, GL_TIMESTAMP);
    }

    bool TimestampAvailable(u32 query) {
        if (query == 0) return false;
        GLint listo = GL_FALSE;
        glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &listo);
        return listo == GL_TRUE;
    }

    u64 TimestampNanos(u32 query) {
        if (query == 0) return 0ull;
        GLuint64 nanos = 0;
        glGetQueryObjectui64v(query, GL_QUERY_RESULT, &nanos);
        return static_cast<u64>(nanos);
    }

    // ── Draw ───────────────────────────────────────────────────────────────
    void DrawIndexed(u32 indexCount) {
        ++g_counters.drawCalls;
        g_counters.triangles += indexCount / 3u;
        glDrawElements(GL_TRIANGLES, (GLsizei)indexCount, GL_UNSIGNED_INT, nullptr);
    }

    void DrawArrays(u32 vertexCount) {
        ++g_counters.drawCalls;
        g_counters.triangles += vertexCount / 3u;
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertexCount);
    }

}
