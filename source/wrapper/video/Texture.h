#ifndef TEXTURE_H
#define TEXTURE_H

#include "GLIncludes.h"

class Texture {
protected:
   GLuint textureID;
   GLenum target;

public:
   Texture(GLenum target);

   Texture(Texture &&other);

   virtual ~Texture();

   GLuint id() const {
      return textureID;
   }

   void bind();

   void unbind();
};

#endif
