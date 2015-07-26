#include "Mesh.h"

#include "FancyAssert.h"

Mesh::Mesh(const float *vertices, unsigned int numVertices,
           const unsigned int *indices, unsigned int numIndices, unsigned int dimensionality)
   : numIndices(numIndices), dimensionality(dimensionality) {
   ASSERT(vertices || numVertices == 0, "numVertices > 0, but no vertices provided");
   ASSERT(indices || numIndices == 0, "numIndices > 0, but no indices provided");
   ASSERT(dimensionality >= 1 && dimensionality <= 4, "dimensionality must be between 1 and 4 (inclusive)");

   glGenBuffers(1, &vbo);
   glGenBuffers(1, &ibo);

   glBindBuffer(GL_ARRAY_BUFFER, vbo);
   glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(float), vertices, GL_STATIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(unsigned int), indices, GL_STATIC_DRAW);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

Mesh::Mesh(Mesh &&other)
   : vbo(other.vbo), ibo(other.ibo), numIndices(other.numIndices), dimensionality(other.dimensionality) {
   other.vbo = other.ibo = other.numIndices = other.dimensionality = 0;
}

Mesh::~Mesh() {
   glDeleteBuffers(1, &vbo);
   glDeleteBuffers(1, &ibo);
}
