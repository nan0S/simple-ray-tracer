#pragma once

#include <GL/glew.h>

namespace Graphics
{
   GLuint loadGraphicsShader(const char *vertex_path, const char *fragment_path);
   GLuint createGraphicsShader(const char *vertex_source, const char *fragment_source);
   GLuint createComputeShader(const char *compute_source);
   void printShaderUniforms(GLuint program);
}
