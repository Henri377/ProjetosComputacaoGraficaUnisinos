#include "SceneObject.h"

/// @brief Ativa a textura e emite a chamada de desenho com os triangulos da submalha.
void SubMesh::draw() const
{
    tex.bind(GL_TEXTURE0);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, nVerts);
    glBindVertexArray(0);
}

/// @brief Libera o VAO, VBO e a textura da GPU e zera os identificadores.
void SubMesh::release()
{
    if (VAO) { glDeleteVertexArrays(1, &VAO); VAO = 0; }
    if (VBO) { glDeleteBuffers(1, &VBO);      VBO = 0; }
    tex.release();
}

/// @brief Libera todas as submalhas do objeto e limpa a lista.
void SceneObject::release()
{
    for (auto& sm : submeshes) sm.release();
    submeshes.clear();
}
