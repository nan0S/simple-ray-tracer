#include "Error.h"

#include <filesystem>

#include <GL/glew.h>

#include "Utils/Log.h"

#define UNUSED __attribute__((unused))
namespace fs = std::filesystem;

void glClearError()
{
   while (glGetError() != GL_NO_ERROR);
}

bool glLogError(UNUSED const char* call, const char* file, UNUSED int line)
{
   GLenum errcode;
   bool good = true;
   const std::string& filename = fs::path(file).filename().string();
   while ((errcode = glGetError()) != GL_NO_ERROR)
   {
      const char* msg = reinterpret_cast<const char*>(gluErrorString(errcode));
      WARNING("[OpenGL Error] ", filename, "::", line, " ", call, " '", msg,
              "' (", errcode, ")");
      good = false;
   }
   return good;
}

