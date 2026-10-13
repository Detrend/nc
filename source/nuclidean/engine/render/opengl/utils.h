// Project Nuclidean Source File
#pragma once

#include <config.h>
#include <engine/render/opengl/gl_types.h>

namespace nc::render::debug
{

#if NC_DEBUG_DRAW
void gl_debug_message(GLenum, GLenum, GLuint, GLenum severity, GLsizei, const GLchar* message, const void*);
#endif

}
