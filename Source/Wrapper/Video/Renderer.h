#ifndef RENDERER_H
#define RENDERER_H

#include "GLIncludes.h"
#include "Wrapper/Video/Model.h"
#include "Wrapper/Video/Texture.h"

#include "GBC/LCDController.h"

#include <array>

class Renderer {
private:
   Model model;
   Texture texture;

public:
   Renderer(int width, int height);

   virtual ~Renderer();

   void onFramebufferSizeChange(int width, int height);

   void draw(const std::array<GBC::Pixel, GBC::kScreenWidth * GBC::kScreenHeight> &pixels);
};

#endif
