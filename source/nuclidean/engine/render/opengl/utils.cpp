#include <engine/render/opengl/utils.h>

#include <common.h>
#include <cvars.h>

#include <glad/glad.h>

namespace nc::render::debug
{

//==============================================================================
#if NC_DEBUG_DRAW
void gl_debug_message(GLenum, GLenum, GLuint, GLenum severity, GLsizei, const GLchar* message, const void*)
{
  s32 numeric_severity = 0;
  switch (severity)
  {
    case GL_DEBUG_SEVERITY_NOTIFICATION: numeric_severity = 0; break;
    case GL_DEBUG_SEVERITY_LOW:          numeric_severity = 1; break;
    case GL_DEBUG_SEVERITY_MEDIUM:       numeric_severity = 2; break;
    case GL_DEBUG_SEVERITY_HIGH:         numeric_severity = 3; break;
    default: nc_assert(false, "Unknown severity");             break;
  }

  if (numeric_severity < CVars::opengl_debug_severity)
    return;

  nc_log("[render][opengl] debug message: '{0}'", message);
}
#endif

}
