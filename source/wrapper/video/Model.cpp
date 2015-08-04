#include "Model.h"

Model::Model(Mesh &&mesh, ShaderProgram &&program)
   : mesh(std::move(mesh)), program(std::move(program)) {
   glGenVertexArrays(1, &vao);

   glBindVertexArray(vao);

   glBindBuffer(GL_ARRAY_BUFFER, this->mesh.getVBO());
   glEnableVertexAttribArray(ShaderAttributes::POSITION);
   glVertexAttribPointer(ShaderAttributes::POSITION, this->mesh.getDimensionality(), GL_FLOAT, GL_FALSE, 0, 0);

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->mesh.getIBO());
}

Model::~Model() {
   glDeleteVertexArrays(1, &vao);
}

void Model::draw() {
   glBindVertexArray(vao);

   program.commit();
   glDrawElements(GL_TRIANGLES, mesh.getNumIndices(), GL_UNSIGNED_INT, 0);
}
