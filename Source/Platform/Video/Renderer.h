#pragma once

#include "Emulator/Emulator.h"

#include "Platform/Video/Model.h"
#include "Platform/Video/Texture.h"

class Renderer
{
public:
   Renderer(int width, int height);

   void onFramebufferSizeChanged(int width, int height);
   void draw(const DotMatrix::PixelArray& pixels);

   GLuint getTextureId() const
   {
      return texture.id();
   }

private:
   Model model;
   Texture texture;
};
