#ifndef MODEL_H
#define MODEL_H

#include "GLIncludes.h"
#include "Mesh.h"
#include "ShaderProgram.h"

class Model {
private:
   GLuint vao;
   Mesh mesh;
   ShaderProgram program;

public:
   Model(Mesh &&mesh, ShaderProgram &&program);

   virtual ~Model();

   void draw();

   ShaderProgram& getProgram() {
      return program;
   }

   GLuint getVAO() const {
      return vao;
   }
};

#endif
