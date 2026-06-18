#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "Material.h"
#include "Texture2D.h"
#include "Trajectory.h"

/**
 * @brief Submalha geometrica com VAO/VBO proprios, material e textura.
 *
 * Cada SubMesh representa um grupo de faces de um arquivo OBJ que compartilha
 * o mesmo material. Contem os buffers OpenGL necessarios para renderizacao
 * e os parametros de iluminacao Phong associados.
 */
struct SubMesh {
    /// @brief Vertex Array Object OpenGL desta submalha.
    GLuint    VAO    = 0;

    /// @brief Vertex Buffer Object OpenGL desta submalha.
    GLuint    VBO    = 0;

    /// @brief Numero de vertices armazenados no VBO.
    int       nVerts = 0;

    /// @brief Material Phong associado a esta submalha.
    Material  mat;

    /// @brief Textura difusa associada a esta submalha.
    Texture2D tex;

    /// @brief Ativa a textura e emite a chamada de desenho com os triangulos da submalha.
    void draw() const;

    /// @brief Libera o VAO, VBO e a textura da GPU.
    void release();
};

/**
 * @brief Objeto da cena composto por uma ou mais SubMeshes.
 *
 * Agrega as submalhas carregadas de um arquivo OBJ com transformacoes
 * (posicao, escala, rotacao) e suporte a trajetoria parametrica.
 */
struct SceneObject {
    /// @brief Nome de identificacao do objeto (exibido na selecao).
    std::string          name;

    /// @brief Lista de submalhas que compoem o objeto.
    std::vector<SubMesh> submeshes;

    /// @brief Posicao do objeto no espaco do mundo.
    glm::vec3            position{0};

    /// @brief Fator de escala uniforme ou por eixo do objeto.
    glm::vec3            scale{1};

    /// @brief Rotacao base lida do arquivo de cena (graus, por eixo XYZ).
    glm::vec3            baseRot{0};

    /// @brief Rotacao dinamica aplicada em tempo de execucao (graus, por eixo XYZ).
    glm::vec3            rotation{0};

    /// @brief Trajetoria parametrica do objeto (Catmull-Rom).
    Trajectory           trajectory;

    /// @brief Caminho para o arquivo de trajetoria associado a este objeto.
    std::string          trajectoryFile;

    /// @brief Libera todas as submalhas e limpa a lista.
    void release();
};
