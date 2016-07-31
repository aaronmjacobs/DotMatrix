#ifndef RENDERER_H
#define RENDERER_H

#include "GLIncludes.h"
#include "wrapper/video/Model.h"
#include "wrapper/video/Texture.h"

#include "gbc/LCDController.h"

#include <array>

class Renderer {
private:
   Model model;
   Texture texture;

public:
   Renderer();

   virtual ~Renderer();

   void onFramebufferSizeChange(int width, int height);

   void draw(const std::array<GBC::Pixel, GBC::kScreenWidth * GBC::kScreenHeight> &pixels);
};

#endif
