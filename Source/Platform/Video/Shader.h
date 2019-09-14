#pragma once

#include <glad/glad.h>

class Shader
{
public:
   Shader(const GLenum type);
   ~Shader();

   bool compile(const char* source);

   GLuint getID() const
   {
      return id;
   }

   GLenum getType() const
   {
      return type;
   }

private:
   const GLuint id;
   const GLenum type;
};
