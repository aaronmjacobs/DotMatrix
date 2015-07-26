#ifndef MESH_H
#define MESH_H

#include "GLIncludes.h"

class Mesh {
private:
   GLuint vbo;
   GLuint ibo;
   unsigned int numIndices;
   unsigned int dimensionality;

public:
   Mesh(const float *vertices, unsigned int numVertices,
        const unsigned int *indices, unsigned int numIndices, unsigned int dimensionality = 3);

   Mesh(Mesh &&other);

   virtual ~Mesh();

   GLuint getVBO() const {
      return vbo;
   }

   GLuint getIBO() const {
      return ibo;
   }

   unsigned int getNumIndices() const {
      return numIndices;
   }

   unsigned int getDimensionality() const {
      return dimensionality;
   }
};

#endif
