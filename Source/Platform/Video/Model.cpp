#include "Platform/Video/Model.h"

Model::Model(Mesh&& inMesh, ShaderProgram&& inProgram)
   : mesh(std::move(inMesh))
   , program(std::move(inProgram))
{
   glGenVertexArrays(1, &vao);

   glBindVertexArray(vao);

   glBindBuffer(GL_ARRAY_BUFFER, mesh.getVBO());
   glEnableVertexAttribArray(ShaderAttribute::kPosition);
   glVertexAttribPointer(ShaderAttribute::kPosition, mesh.getDimensionality(), GL_FLOAT, GL_FALSE, 0, 0);

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.getIBO());
}

Model::~Model()
{
   glDeleteVertexArrays(1, &vao);
}

void Model::draw()
{
   glBindVertexArray(vao);

   program.commit();
   glDrawElements(GL_TRIANGLES, mesh.getNumIndices(), GL_UNSIGNED_INT, 0);
}
