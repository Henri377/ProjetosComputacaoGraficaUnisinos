#pragma once
#include <glad/glad.h>
#include "Light.h"
#include <vector>

/**
 * @brief Compila os shaders GLSL embutidos (vertex + fragment) e linka o programa.
 *
 * O vertex shader transforma posicoes para clip space e propaga UV e normais.
 * O fragment shader implementa iluminacao Phong com suporte a ate 4 luzes.
 *
 * @return Identificador OpenGL do programa de shader linkado.
 */
GLuint compileShaders();

/**
 * @brief Envia os dados das luzes ativas para os uniforms do shader.
 *
 * Percorre o vetor de luzes, ignora as desativadas e preenche os arrays
 * lightPos, lightColor e lightIntensity no shader, atualizando numLights.
 * Suporta no maximo 4 luzes simultaneas.
 *
 * @param shader Identificador OpenGL do programa de shader ativo.
 * @param lights Vetor de luzes da cena (habilitadas e desabilitadas).
 */
void uploadLights(GLuint shader, const std::vector<Light>& lights);
