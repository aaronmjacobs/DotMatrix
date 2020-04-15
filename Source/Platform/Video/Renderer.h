#pragma once

#include "Emulator.h"

#include "Platform/Video/Model.h"
#include "Platform/Video/Texture.h"

class Renderer
{
public:
   Renderer(int width, int height);

   void onFramebufferSizeChanged(int width, int height);
   void draw(const PixelArray& pixels);

   GLuint getTextureId() const
   {
      return texture.id();
   }

private:
   Model model;
   Texture texture;
};
