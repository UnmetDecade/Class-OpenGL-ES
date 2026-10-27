#pragma once

#include <glad/gles2.h>

#include <cstdio>
#include <cstdlib>

#ifdef DEBUG

namespace engine::detail
{

inline const char* GLErrorToString(GLenum error)
{
    switch (error)
    {
    case GL_INVALID_ENUM:                  return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:                 return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:             return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:                 return "GL_OUT_OF_MEMORY";
    default:                               return "GL_UNKNOWN_ERROR";
    }
}

// Errors left over from previous calls must be drained, otherwise they would
// be misattributed to the call that is about to be checked.
inline void GLClearErrors()
{
    while (glGetError() != GL_NO_ERROR)
    {
    }
}

inline void GLCheckErrors(const char* call, const char* file, int line)
{
    bool failed = false;

    while (GLenum error = glGetError())
    {
        std::fprintf(stderr,
                     "[OpenGL Error] %s:%d\n    %s\n    %s (0x%04X)\n",
                     file, line, call, GLErrorToString(error), error);
        failed = true;
    }

    if (failed)
    {
        std::abort();
    }
}

} // namespace engine::detail

#define GLCall(call)                                     \
    do                                                   \
    {                                                    \
        ::engine::detail::GLClearErrors();               \
        call;                                            \
        ::engine::detail::GLCheckErrors(#call, __FILE__, __LINE__); \
    } while (0)

#else

#define GLCall(call) call

#endif
