#pragma once

#include "GBC/LCDController.h"

#include "Platform/Video/Model.h"
#include "Platform/Video/Texture.h"

#include <array>

class Renderer
{
public:
   Renderer(int width, int height);

   void onFramebufferSizeChanged(int width, int height);
   void draw(const std::array<GBC::Pixel, GBC::kScreenWidth * GBC::kScreenHeight>& pixels);

   GLuint getTextureId() const
   {
      return texture.id();
   }

private:
   Model model;
   Texture texture;
};
