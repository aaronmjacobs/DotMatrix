#pragma once

#if !GBC_WITH_UI
#   error "Including UI config header, but GBC_WITH_UI is not set!"
#endif // !GBC_WITH_UI

class Renderer;

namespace GBC
{
class GameBoy;
} // namespace GBC

struct GLFWwindow;

class UI
{
public:
   static int getDesiredWindowWidth();
   static int getDesiredWindowHeight();

   UI(GLFWwindow* window);
   ~UI();

   void render(GBC::GameBoy& gameBoy, const Renderer& renderer);

private:
   void renderScreen(const Renderer& renderer) const;
   void renderJoypad(GBC::GameBoy& gameBoy) const;
   void renderCPU(GBC::GameBoy& gameBoy) const;
   void renderMemory(GBC::GameBoy& gameBoy) const;
   void renderSoundController(GBC::GameBoy& gameBoy) const;
};
