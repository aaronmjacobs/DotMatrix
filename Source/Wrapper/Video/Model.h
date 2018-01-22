#ifndef MODEL_H
#define MODEL_H

#include "GLIncludes.h"

#include "Wrapper/Video/Mesh.h"
#include "Wrapper/Video/ShaderProgram.h"

class Model {
public:
   Model(Mesh&& inMesh, ShaderProgram&& inProgram);
   ~Model();

   void draw();

   ShaderProgram& getProgram() {
      return program;
   }

   GLuint getVAO() const {
      return vao;
   }

private:
   GLuint vao;
   Mesh mesh;
   ShaderProgram program;
};

#endif
