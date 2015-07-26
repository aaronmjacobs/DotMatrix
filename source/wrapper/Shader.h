#ifndef SHADER_H
#define SHADER_H

#include "GLIncludes.h"

class Shader {
protected:
   const GLuint id;
   const GLenum type;

public:
   Shader(const GLenum type);

   virtual ~Shader();

   bool compile(const char *source);

   GLuint getID() const {
      return id;
   }

   GLenum getType() const {
      return type;
   }
};

#endif
