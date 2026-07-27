// RHIOpenGL.cpp
//  Backend OpenGL 4.5 del RHI. Este es el ÚNICO translation unit del proyecto
//  que incluye glad: ninguna llamada gl* ni enum GL_* puede existir fuera de
//  acá. Un backend Vulkan futuro sería otro .cpp implementando RHI.h.
#include "RHI.h"
#include "Assert.h"

#include <glad/gl.h>

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

    // ── Inicialización / contexto ───────────────────────────────────────────
    bool Initialize(ProcAddressLoader loader) {
        EFCOM_ASSERT(loader != nullptr, "Initialize: loader no puede ser null");
        return gladLoadGL((GLADloadfunc)loader) != 0;
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

    void Clear(ClearMask mask) {
        GLbitfield bits = 0;
        if ((u32)mask & (u32)ClearMask::Color) bits |= GL_COLOR_BUFFER_BIT;
        if ((u32)mask & (u32)ClearMask::Depth) bits |= GL_DEPTH_BUFFER_BIT;
        glClear(bits);
    }

    void SetDepthTest(bool enabled) {
        if (enabled) glEnable(GL_DEPTH_TEST);
        else         glDisable(GL_DEPTH_TEST);
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

    i32 GetUniformLocation(u32 program, const char* name) {
        return glGetUniformLocation(program, name);
    }

    void SetUniformInt(i32 location, i32 value)          { glUniform1i(location, value); }
    void SetUniformFloat(i32 location, f32 value)        { glUniform1f(location, value); }
    void SetUniformVec3(i32 location, const f32* values3)  { glUniform3fv(location, 1, values3); }
    void SetUniformMat4(i32 location, const f32* values16) { glUniformMatrix4fv(location, 1, GL_FALSE, values16); }

    // ── Texturas 2D ────────────────────────────────────────────────────────
    u32 CreateTexture2D(const Texture2DDesc& desc, const void* pixels) {
        const GLFormat fmt = to_gl(desc.format);

        u32 id = 0;
        glGenTextures(1, &id);
        EFCOM_ASSERT(id != 0, "CreateTexture2D: glGenTextures fallo (sin contexto GL)");

        glBindTexture(GL_TEXTURE_2D, id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, to_gl(desc.minFilter));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, to_gl(desc.magFilter));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, to_gl(desc.wrapS));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, to_gl(desc.wrapT));
        if (desc.wrapS == TextureWrap::ClampToBorder || desc.wrapT == TextureWrap::ClampToBorder) {
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, desc.borderColor);
        }

        glTexImage2D(GL_TEXTURE_2D, 0, (GLint)fmt.internalFormat,
                     (GLsizei)desc.width, (GLsizei)desc.height, 0,
                     fmt.pixelFormat, fmt.pixelType, pixels);
        if (desc.generateMipmaps) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        return id;
    }

    void DestroyTexture(u32 texture) { glDeleteTextures(1, &texture); }

    void BindTexture2D(u32 texture, u32 unit) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, texture);
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

    // ── Draw ───────────────────────────────────────────────────────────────
    void DrawIndexed(u32 indexCount) {
        glDrawElements(GL_TRIANGLES, (GLsizei)indexCount, GL_UNSIGNED_INT, nullptr);
    }

    void DrawArrays(u32 vertexCount) {
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertexCount);
    }

}
