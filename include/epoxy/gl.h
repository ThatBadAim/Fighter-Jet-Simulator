#pragma once

#if defined(_WIN32) || !defined(__has_include) || !__has_include(<epoxy/gl.h>)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>
#include <SDL3/SDL.h>
#include <cstddef>

#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
typedef char GLchar;

#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_FRAMEBUFFER 0x8D40
#define GL_RENDERBUFFER 0x8D41
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#define GL_DEPTH24_STENCIL8 0x88F0
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#define GL_TEXTURE0 0x84C0
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_MULTISAMPLE 0x809D
#endif

// Function pointer declarations
typedef void (APIENTRYP PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint *arrays);
typedef void (APIENTRYP PFNGLBINDVERTEXARRAYPROC)(GLuint array);
typedef void (APIENTRYP PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint *arrays);
typedef void (APIENTRYP PFNGLGENBUFFERSPROC)(GLsizei n, GLuint *buffers);
typedef void (APIENTRYP PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void (APIENTRYP PFNGLBUFFERDATAPROC)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void (APIENTRYP PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint *buffers);
typedef void (APIENTRYP PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void (APIENTRYP PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef void (APIENTRYP PFNGLDISABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef GLuint (APIENTRYP PFNGLCREATESHADERPROC)(GLenum type);
typedef void (APIENTRYP PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (APIENTRYP PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef void (APIENTRYP PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRYP PFNGLDELETESHADERPROC)(GLuint shader);
typedef GLuint (APIENTRYP PFNGLCREATEPROGRAMPROC)(void);
typedef void (APIENTRYP PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void (APIENTRYP PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void (APIENTRYP PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRYP PFNGLUSEPROGRAMPROC)(GLuint program);
typedef void (APIENTRYP PFNGLDELETEPROGRAMPROC)(GLuint program);
typedef GLint (APIENTRYP PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar *name);
typedef void (APIENTRYP PFNGLUNIFORMMATRIX4FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORMMATRIX3FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORM3FVPROC)(GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORM3FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRYP PFNGLUNIFORM4FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (APIENTRYP PFNGLUNIFORM1FPROC)(GLint location, GLfloat v0);
typedef void (APIENTRYP PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
typedef void (APIENTRYP PFNGLUNIFORM2FPROC)(GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRYP PFNGLGENFRAMEBUFFERSPROC)(GLsizei n, GLuint *framebuffers);
typedef void (APIENTRYP PFNGLBINDFRAMEBUFFERPROC)(GLenum target, GLuint framebuffer);
typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRYP PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei n, const GLuint *framebuffers);
typedef GLenum (APIENTRYP PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum target);
typedef void (APIENTRYP PFNGLGENRENDERBUFFERSPROC)(GLsizei n, GLuint *renderbuffers);
typedef void (APIENTRYP PFNGLBINDRENDERBUFFERPROC)(GLenum target, GLuint renderbuffer);
typedef void (APIENTRYP PFNGLRENDERBUFFERSTORAGEPROC)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRYP PFNGLFRAMEBUFFERRENDERBUFFERPROC)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef void (APIENTRYP PFNGLDELETERENDERBUFFERSPROC)(GLsizei n, const GLuint *renderbuffers);
typedef void (APIENTRYP PFNGLGENERATEMIPMAPPROC)(GLenum target);
typedef void (APIENTRYP PFNGLACTIVETEXTUREPROC)(GLenum texture);

struct FastJetGLFunctions {
    PFNGLGENVERTEXARRAYSPROC genVertexArrays = nullptr;
    PFNGLBINDVERTEXARRAYPROC bindVertexArray = nullptr;
    PFNGLDELETEVERTEXARRAYSPROC deleteVertexArrays = nullptr;
    PFNGLGENBUFFERSPROC genBuffers = nullptr;
    PFNGLBINDBUFFERPROC bindBuffer = nullptr;
    PFNGLBUFFERDATAPROC bufferData = nullptr;
    PFNGLDELETEBUFFERSPROC deleteBuffers = nullptr;
    PFNGLVERTEXATTRIBPOINTERPROC vertexAttribPointer = nullptr;
    PFNGLENABLEVERTEXATTRIBARRAYPROC enableVertexAttribArray = nullptr;
    PFNGLDISABLEVERTEXATTRIBARRAYPROC disableVertexAttribArray = nullptr;
    PFNGLCREATESHADERPROC createShader = nullptr;
    PFNGLSHADERSOURCEPROC shaderSource = nullptr;
    PFNGLCOMPILESHADERPROC compileShader = nullptr;
    PFNGLGETSHADERIVPROC getShaderiv = nullptr;
    PFNGLGETSHADERINFOLOGPROC getShaderInfoLog = nullptr;
    PFNGLDELETESHADERPROC deleteShader = nullptr;
    PFNGLCREATEPROGRAMPROC createProgram = nullptr;
    PFNGLATTACHSHADERPROC attachShader = nullptr;
    PFNGLLINKPROGRAMPROC linkProgram = nullptr;
    PFNGLGETPROGRAMIVPROC getProgramiv = nullptr;
    PFNGLGETPROGRAMINFOLOGPROC getProgramInfoLog = nullptr;
    PFNGLUSEPROGRAMPROC useProgram = nullptr;
    PFNGLDELETEPROGRAMPROC deleteProgram = nullptr;
    PFNGLGETUNIFORMLOCATIONPROC getUniformLocation = nullptr;
    PFNGLUNIFORMMATRIX4FVPROC uniformMatrix4fv = nullptr;
    PFNGLUNIFORMMATRIX3FVPROC uniformMatrix3fv = nullptr;
    PFNGLUNIFORM3FVPROC uniform3fv = nullptr;
    PFNGLUNIFORM3FPROC uniform3f = nullptr;
    PFNGLUNIFORM4FPROC uniform4f = nullptr;
    PFNGLUNIFORM1FPROC uniform1f = nullptr;
    PFNGLUNIFORM1IPROC uniform1i = nullptr;
    PFNGLUNIFORM2FPROC uniform2f = nullptr;
    PFNGLGENFRAMEBUFFERSPROC genFramebuffers = nullptr;
    PFNGLBINDFRAMEBUFFERPROC bindFramebuffer = nullptr;
    PFNGLFRAMEBUFFERTEXTURE2DPROC framebufferTexture2D = nullptr;
    PFNGLDELETEFRAMEBUFFERSPROC deleteFramebuffers = nullptr;
    PFNGLCHECKFRAMEBUFFERSTATUSPROC checkFramebufferStatus = nullptr;
    PFNGLGENRENDERBUFFERSPROC genRenderbuffers = nullptr;
    PFNGLBINDRENDERBUFFERPROC bindRenderbuffer = nullptr;
    PFNGLRENDERBUFFERSTORAGEPROC renderbufferStorage = nullptr;
    PFNGLFRAMEBUFFERRENDERBUFFERPROC framebufferRenderbuffer = nullptr;
    PFNGLDELETERENDERBUFFERSPROC deleteRenderbuffers = nullptr;
    PFNGLGENERATEMIPMAPPROC generateMipmap = nullptr;
    PFNGLACTIVETEXTUREPROC activeTexture = nullptr;

    static FastJetGLFunctions& instance() {
        static FastJetGLFunctions fn;
        return fn;
    }

    void load() {
        genVertexArrays = (PFNGLGENVERTEXARRAYSPROC)SDL_GL_GetProcAddress("glGenVertexArrays");
        bindVertexArray = (PFNGLBINDVERTEXARRAYPROC)SDL_GL_GetProcAddress("glBindVertexArray");
        deleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)SDL_GL_GetProcAddress("glDeleteVertexArrays");
        genBuffers = (PFNGLGENBUFFERSPROC)SDL_GL_GetProcAddress("glGenBuffers");
        bindBuffer = (PFNGLBINDBUFFERPROC)SDL_GL_GetProcAddress("glBindBuffer");
        bufferData = (PFNGLBUFFERDATAPROC)SDL_GL_GetProcAddress("glBufferData");
        deleteBuffers = (PFNGLDELETEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteBuffers");
        vertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)SDL_GL_GetProcAddress("glVertexAttribPointer");
        enableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)SDL_GL_GetProcAddress("glEnableVertexAttribArray");
        disableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)SDL_GL_GetProcAddress("glDisableVertexAttribArray");
        createShader = (PFNGLCREATESHADERPROC)SDL_GL_GetProcAddress("glCreateShader");
        shaderSource = (PFNGLSHADERSOURCEPROC)SDL_GL_GetProcAddress("glShaderSource");
        compileShader = (PFNGLCOMPILESHADERPROC)SDL_GL_GetProcAddress("glCompileShader");
        getShaderiv = (PFNGLGETSHADERIVPROC)SDL_GL_GetProcAddress("glGetShaderiv");
        getShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)SDL_GL_GetProcAddress("glGetShaderInfoLog");
        deleteShader = (PFNGLDELETESHADERPROC)SDL_GL_GetProcAddress("glDeleteShader");
        createProgram = (PFNGLCREATEPROGRAMPROC)SDL_GL_GetProcAddress("glCreateProgram");
        attachShader = (PFNGLATTACHSHADERPROC)SDL_GL_GetProcAddress("glAttachShader");
        linkProgram = (PFNGLLINKPROGRAMPROC)SDL_GL_GetProcAddress("glLinkProgram");
        getProgramiv = (PFNGLGETPROGRAMIVPROC)SDL_GL_GetProcAddress("glGetProgramiv");
        getProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)SDL_GL_GetProcAddress("glGetProgramInfoLog");
        useProgram = (PFNGLUSEPROGRAMPROC)SDL_GL_GetProcAddress("glUseProgram");
        deleteProgram = (PFNGLDELETEPROGRAMPROC)SDL_GL_GetProcAddress("glDeleteProgram");
        getUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)SDL_GL_GetProcAddress("glGetUniformLocation");
        uniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)SDL_GL_GetProcAddress("glUniformMatrix4fv");
        uniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)SDL_GL_GetProcAddress("glUniformMatrix3fv");
        uniform3fv = (PFNGLUNIFORM3FVPROC)SDL_GL_GetProcAddress("glUniform3fv");
        uniform3f = (PFNGLUNIFORM3FPROC)SDL_GL_GetProcAddress("glUniform3f");
        uniform4f = (PFNGLUNIFORM4FPROC)SDL_GL_GetProcAddress("glUniform4f");
        uniform1f = (PFNGLUNIFORM1FPROC)SDL_GL_GetProcAddress("glUniform1f");
        uniform1i = (PFNGLUNIFORM1IPROC)SDL_GL_GetProcAddress("glUniform1i");
        uniform2f = (PFNGLUNIFORM2FPROC)SDL_GL_GetProcAddress("glUniform2f");
        genFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glGenFramebuffers");
        bindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)SDL_GL_GetProcAddress("glBindFramebuffer");
        framebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)SDL_GL_GetProcAddress("glFramebufferTexture2D");
        deleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteFramebuffers");
        checkFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)SDL_GL_GetProcAddress("glCheckFramebufferStatus");
        genRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)SDL_GL_GetProcAddress("glGenRenderbuffers");
        bindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)SDL_GL_GetProcAddress("glBindRenderbuffer");
        renderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)SDL_GL_GetProcAddress("glRenderbufferStorage");
        framebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)SDL_GL_GetProcAddress("glFramebufferRenderbuffer");
        deleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteRenderbuffers");
        generateMipmap = (PFNGLGENERATEMIPMAPPROC)SDL_GL_GetProcAddress("glGenerateMipmap");
        activeTexture = (PFNGLACTIVETEXTUREPROC)SDL_GL_GetProcAddress("glActiveTexture");
    }
};

#define glGenVertexArrays FastJetGLFunctions::instance().genVertexArrays
#define glBindVertexArray FastJetGLFunctions::instance().bindVertexArray
#define glDeleteVertexArrays FastJetGLFunctions::instance().deleteVertexArrays
#define glGenBuffers FastJetGLFunctions::instance().genBuffers
#define glBindBuffer FastJetGLFunctions::instance().bindBuffer
#define glBufferData FastJetGLFunctions::instance().bufferData
#define glDeleteBuffers FastJetGLFunctions::instance().deleteBuffers
#define glVertexAttribPointer FastJetGLFunctions::instance().vertexAttribPointer
#define glEnableVertexAttribArray FastJetGLFunctions::instance().enableVertexAttribArray
#define glDisableVertexAttribArray FastJetGLFunctions::instance().disableVertexAttribArray
#define glCreateShader FastJetGLFunctions::instance().createShader
#define glShaderSource FastJetGLFunctions::instance().shaderSource
#define glCompileShader FastJetGLFunctions::instance().compileShader
#define glGetShaderiv FastJetGLFunctions::instance().getShaderiv
#define glGetShaderInfoLog FastJetGLFunctions::instance().getShaderInfoLog
#define glDeleteShader FastJetGLFunctions::instance().deleteShader
#define glCreateProgram FastJetGLFunctions::instance().createProgram
#define glAttachShader FastJetGLFunctions::instance().attachShader
#define glLinkProgram FastJetGLFunctions::instance().linkProgram
#define glGetProgramiv FastJetGLFunctions::instance().getProgramiv
#define glGetProgramInfoLog FastJetGLFunctions::instance().getProgramInfoLog
#define glUseProgram FastJetGLFunctions::instance().useProgram
#define glDeleteProgram FastJetGLFunctions::instance().deleteProgram
#define glGetUniformLocation FastJetGLFunctions::instance().getUniformLocation
#define glUniformMatrix4fv FastJetGLFunctions::instance().uniformMatrix4fv
#define glUniformMatrix3fv FastJetGLFunctions::instance().uniformMatrix3fv
#define glUniform3fv FastJetGLFunctions::instance().uniform3fv
#define glUniform3f FastJetGLFunctions::instance().uniform3f
#define glUniform4f FastJetGLFunctions::instance().uniform4f
#define glUniform1f FastJetGLFunctions::instance().uniform1f
#define glUniform1i FastJetGLFunctions::instance().uniform1i
#define glUniform2f FastJetGLFunctions::instance().uniform2f
#define glGenFramebuffers FastJetGLFunctions::instance().genFramebuffers
#define glBindFramebuffer FastJetGLFunctions::instance().bindFramebuffer
#define glFramebufferTexture2D FastJetGLFunctions::instance().framebufferTexture2D
#define glDeleteFramebuffers FastJetGLFunctions::instance().deleteFramebuffers
#define glCheckFramebufferStatus FastJetGLFunctions::instance().checkFramebufferStatus
#define glGenRenderbuffers FastJetGLFunctions::instance().genRenderbuffers
#define glBindRenderbuffer FastJetGLFunctions::instance().bindRenderbuffer
#define glRenderbufferStorage FastJetGLFunctions::instance().renderbufferStorage
#define glFramebufferRenderbuffer FastJetGLFunctions::instance().framebufferRenderbuffer
#define glDeleteRenderbuffers FastJetGLFunctions::instance().deleteRenderbuffers
#define glGenerateMipmap FastJetGLFunctions::instance().generateMipmap
#define glActiveTexture FastJetGLFunctions::instance().activeTexture

#endif
