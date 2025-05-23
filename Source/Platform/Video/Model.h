#pragma once

#include "Platform/Video/Mesh.h"
#include "Platform/Video/ShaderProgram.h"

#include <glad/gl.h>

class Model
{
public:
   Model(Mesh&& inMesh, ShaderProgram&& inProgram);
   ~Model();

   void draw();

   ShaderProgram& getProgram()
   {
      return program;
   }

   GLuint getVAO() const
   {
      return vao;
   }

private:
   GLuint vao;
   Mesh mesh;
   ShaderProgram program;
};
