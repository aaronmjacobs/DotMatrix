#ifndef RENDERER_H
#define RENDERER_H

#include "GLIncludes.h"
#include "Wrapper/Video/Model.h"
#include "Wrapper/Video/Texture.h"

#include "GBC/LCDController.h"

#include <array>

class Renderer {
public:
   Renderer(int width, int height);

   void onFramebufferSizeChanged(int width, int height);
   void draw(const std::array<GBC::Pixel, GBC::kScreenWidth * GBC::kScreenHeight>& pixels);

private:
   Model model;
   Texture texture;
};

#endif
