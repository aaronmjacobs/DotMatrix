#pragma once

#include <glad/gl.h>

class Texture
{
public:
   Texture(GLenum textureTarget);
   Texture(Texture&& other);
   ~Texture();

   GLuint id() const
   {
      return textureID;
   }

   void bind();
   void unbind();

private:
   GLuint textureID;
   GLenum target;
};
