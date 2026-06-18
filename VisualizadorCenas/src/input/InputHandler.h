#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include "Camera.h"
#include "SceneObject.h"
#include "Light.h"

/// @brief Eixo de rotacao automatica do objeto selecionado.
enum class RotationAxis { None, X, Y, Z };

/**
 * @brief Estado global compartilhado entre o loop principal e os callbacks GLFW.
 *
 * Centraliza todos os dados mutaveis da aplicacao: camera, objetos, luzes,
 * shader ativo, objeto selecionado e eixo de rotacao automatica.
 */
struct AppState {
    Camera               camera;           ///< Camera FPS da cena.
    std::vector<SceneObject> objects;      ///< Lista de objetos carregados.
    std::vector<Light>   lights;           ///< Lista de fontes de luz.
    GLuint               shader   = 0;    ///< Programa de shader ativo.
    int                  selected = 0;    ///< Indice do objeto selecionado.
    RotationAxis         rotAxis  = RotationAxis::None; ///< Eixo de rotacao automatica.
    float                delta    = 0.0f; ///< Delta time do frame atual (s).
    float                lastFrame = 0.0f; ///< Timestamp do frame anterior (s).
};

/// @brief Instancia global do estado da aplicacao, definida em InputHandler.cpp.
extern AppState g_app;

/**
 * @brief Callback de teclado GLFW — trata selecao, luzes, rotacao, trajetoria e transformacoes.
 * @param win    Janela GLFW que gerou o evento.
 * @param key    Codigo GLFW da tecla pressionada.
 * @param sc     Scancode do SO (ignorado).
 * @param action GLFW_PRESS, GLFW_RELEASE ou GLFW_REPEAT.
 * @param mods   Modificadores como Shift/Ctrl (ignorados).
 */
void keyCallback(GLFWwindow* win, int key, int sc, int action, int mods);

/**
 * @brief Callback de movimento de mouse GLFW — repassa para a camera.
 * @param win Janela GLFW (ignorada).
 * @param x   Posicao horizontal do cursor em pixels.
 * @param y   Posicao vertical do cursor em pixels.
 */
void mouseCallback(GLFWwindow* win, double x, double y);

/**
 * @brief Callback de scroll do mouse GLFW — ajusta o FOV da camera.
 * @param win Janela GLFW (ignorada).
 * @param x   Deslocamento horizontal (ignorado).
 * @param y   Deslocamento vertical (positivo = zoom in).
 */
void scrollCallback(GLFWwindow* win, double x, double y);
