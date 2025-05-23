#pragma once

#include <glad/gl.h>

class Mesh
{
public:
   Mesh(const float* vertices, unsigned int numVertices,
        const unsigned int* indices, unsigned int numIndices, unsigned int dimensionality = 3);
   Mesh(Mesh&& other);
   ~Mesh();

   GLuint getVBO() const
   {
      return vbo;
   }

   GLuint getIBO() const
   {
      return ibo;
   }

   unsigned int getNumIndices() const
   {
      return numIndices;
   }

   unsigned int getDimensionality() const
   {
      return dimensionality;
   }

private:
   GLuint vbo;
   GLuint ibo;
   unsigned int numIndices;
   unsigned int dimensionality;
};
