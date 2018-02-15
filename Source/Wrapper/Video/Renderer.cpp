#include "Renderer.h"

#include "FancyAssert.h"
#include "Shader.h"
#include "ShaderProgram.h"
#include "Texture.h"

#define GLSL(source) "#version 150 core\n" #source

namespace {

const char *kVertShaderSource = GLSL(
   uniform mat4 uProj;

   in vec2 aPosition;

   out vec2 vTexCoord;

   void main() {
      gl_Position = uProj * vec4(aPosition, 0.0, 1.0);
      vTexCoord = (aPosition + 1.0) / 2.0;
      vTexCoord.y = 1.0 - vTexCoord.y; // OpenGL maps textures from bottom to top
   }
);

const char *kFragShaderSource = GLSL(
   uniform sampler2D uTexture;

   in vec2 vTexCoord;

   out vec4 color;

   void main() {
      color = texture(uTexture, vTexCoord);
   }
);

const std::array<float, 8> kVertices = { -1.0f, -1.0f,
                                          1.0f, -1.0f,
                                         -1.0f,  1.0f,
                                          1.0f,  1.0f };

const std::array<unsigned int, 6> kIndices = { 0, 1, 2,
                                               1, 3, 2 };

const GLenum kTextureUnit = 0;

using Mat4 = std::array<float, 16>;

Mat4 ortho(float left, float right, float bottom, float top) {
   Mat4 result {};

   result[0] = 2.0f / (right - left);
   result[5] = 2.0f / (top - bottom);
   result[10] = - 1.0f;
   result[12] = - (right + left) / (right - left);
   result[13] = - (top + bottom) / (top - bottom);
   result[15] = 1.0f;

   return result;
}

} // namespace

Renderer::Renderer(int width, int height)
   : texture(GL_TEXTURE_2D),
     model(Mesh(kVertices.data(), static_cast<unsigned int>(kVertices.size()), kIndices.data(), static_cast<unsigned int>(kIndices.size()), 2), ShaderProgram()) {
   // Back face culling
   glEnable(GL_CULL_FACE);
   glCullFace(GL_BACK);

   // Shaders
   SPtr<Shader> vertShader = std::make_shared<Shader>(GL_VERTEX_SHADER);
   SPtr<Shader> fragShader = std::make_shared<Shader>(GL_FRAGMENT_SHADER);

   if (!vertShader->compile(kVertShaderSource)) {
      ASSERT(false, "Unable to compile vertex shader");
   }
   if (!fragShader->compile(kFragShaderSource)) {
      ASSERT(false, "Unable to compile fragment shader");
   }

   model.getProgram().attach(vertShader);
   model.getProgram().attach(fragShader);
   if (!model.getProgram().link()) {
      ASSERT(false, "Unable to link shader program");
   }

   // Texture
   glActiveTexture(GL_TEXTURE0 + kTextureUnit);
   texture.bind();

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, GBC::kScreenWidth, GBC::kScreenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

   model.getProgram().setUniformValue("uTexture", kTextureUnit);

   onFramebufferSizeChanged(width, height);
}

void Renderer::onFramebufferSizeChanged(int width, int height) {
   ASSERT(width > 0 && height > 0, "Invalid framebuffer size: %d x %d", width, height);

   width = std::max(1, width);
   height = std::max(1, height);
   glViewport(0, 0, width, height);

   static const float kInvGameBoyAspectRatio = static_cast<float>(GBC::kScreenHeight) / GBC::kScreenWidth;
   float framebufferAspectRatio = static_cast<float>(width) / height;
   float aspectRatio = framebufferAspectRatio * kInvGameBoyAspectRatio;
   float invAspectRatio = 1.0f / aspectRatio;

   Mat4 proj = aspectRatio >= 1.0f ? ortho(-aspectRatio, aspectRatio, -1.0f, 1.0f) : ortho(-1.0f, 1.0f, -invAspectRatio, invAspectRatio);
   model.getProgram().setUniformValue("uProj", proj);
}

void Renderer::draw(const std::array<GBC::Pixel, GBC::kScreenWidth * GBC::kScreenHeight>& pixels) {
   glClear(GL_COLOR_BUFFER_BIT);

   glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GBC::kScreenWidth, GBC::kScreenHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
 
   model.draw();
}
