#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/**
 * @brief Camera 3D com suporte a movimento via teclado, mouse e scroll.
 *
 * Implementa uma camera estilo FPS (first-person) com angulos de Euler
 * (yaw/pitch), campo de visao (fov) e matrizes de visao e projecao perspectiva.
 */
class Camera {
public:
    /// @brief Posicao atual da camera no espaco do mundo.
    glm::vec3 position{0, 0, 5};

    /// @brief Angulo horizontal (graus), rotacao ao redor do eixo Y.
    float yaw       = -90.0f;

    /// @brief Angulo vertical (graus), rotacao ao redor do eixo X.
    float pitch     =   0.0f;

    /// @brief Campo de visao vertical em graus.
    float fov       =  45.0f;

    /// @brief Plano de corte proximo (near plane).
    float nearP     =   0.1f;

    /// @brief Plano de corte distante (far plane).
    float farP      = 100.0f;

    /// @brief Velocidade de deslocamento da camera (unidades/segundo).
    float speed     =   2.5f;

    /// @brief Sensibilidade do mouse (graus por pixel).
    float mouseSens =   0.05f;

    /// @brief Inicializa os vetores internos da camera com base nos angulos atuais.
    void init() { updateVectors(); }

    /**
     * @brief Processa entrada de teclado para deslocar a camera.
     * @param w  Janela GLFW da qual a entrada e lida.
     * @param dt Tempo decorrido desde o ultimo frame (delta time), em segundos.
     */
    void processKeyboard(GLFWwindow* w, float dt) {
        float s = speed * dt;
        if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) position += s * front;
        if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) position -= s * front;
        if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS)
            position -= glm::normalize(glm::cross(front, up)) * s;
        if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS)
            position += glm::normalize(glm::cross(front, up)) * s;
    }

    /**
     * @brief Processa o movimento do mouse para orientar a camera.
     * @param xpos Posicao horizontal atual do cursor em pixels.
     * @param ypos Posicao vertical atual do cursor em pixels.
     */
    void processMouse(double xpos, double ypos) {
        if (firstMouse) { lastX = (float)xpos; lastY = (float)ypos; firstMouse = false; }
        float dx = ((float)xpos - lastX) * mouseSens;
        float dy = (lastY - (float)ypos)  * mouseSens;
        lastX = (float)xpos;
        lastY = (float)ypos;
        yaw  += dx;
        pitch = glm::clamp(pitch + dy, -89.0f, 89.0f);
        updateVectors();
    }

    /**
     * @brief Processa o scroll do mouse para ajustar o campo de visao (zoom).
     * @param dy Deslocamento vertical do scroll (positivo = aproximar).
     */
    void processScroll(double dy) { fov = glm::clamp(fov - (float)dy, 1.0f, 89.0f); }

    /**
     * @brief Calcula a matriz de visao com base na posicao e orientacao atuais.
     * @return Matriz 4x4 de visao (lookAt).
     */
    glm::mat4 viewMatrix() const { return glm::lookAt(position, position + front, up); }

    /**
     * @brief Calcula a matriz de projecao perspectiva.
     * @param aspect Razao de aspecto da janela (largura / altura).
     * @return Matriz 4x4 de projecao perspectiva.
     */
    glm::mat4 projMatrix(float aspect) const {
        return glm::perspective(glm::radians(fov), aspect, nearP, farP);
    }

private:
    /// @brief Vetor direcao frontal da camera (derivado de yaw e pitch).
    glm::vec3 front{0, 0, -1};

    /// @brief Vetor "para cima" da camera, recalculado a cada atualizacao.
    glm::vec3 up{0, 1, 0};

    /// @brief Indica se e o primeiro evento de mouse (para evitar salto inicial).
    bool  firstMouse = true;

    /// @brief Ultima posicao X do cursor registrada.
    float lastX = 600.0f;

    /// @brief Ultima posicao Y do cursor registrada.
    float lastY = 400.0f;

    /**
     * @brief Recalcula os vetores front e up a partir dos angulos yaw e pitch atuais.
     */
    void updateVectors() {
        glm::vec3 f;
        f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.y = sin(glm::radians(pitch));
        f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(f);
        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
        up = glm::normalize(glm::cross(right, front));
    }
};
