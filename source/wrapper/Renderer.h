#ifndef RENDERER_H
#define RENDERER_H

#include "GLIncludes.h"
#include "Model.h"
#include "Texture.h"
#include "gbc/Video.h"

#include <array>

class Renderer {
private:
   Model model;
   Texture texture;

public:
   Renderer();

   virtual ~Renderer();

   void onFramebufferSizeChange(int width, int height);

   void draw(const std::array<GBC::Pixel, GBC::SCREEN_WIDTH * GBC::SCREEN_HEIGHT> &pixels);
};

#endif
