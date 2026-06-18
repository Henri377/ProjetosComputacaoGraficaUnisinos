#include "InputHandler.h"
#include "Shader.h"
#include <glm/glm.hpp>
#include <iostream>

AppState g_app;

static const float STEP_T = 0.1f;
static const float STEP_S = 0.05f;
static const float MIN_S  = 0.05f;

// ---------- handlers por categoria ----------

/// @brief Seleciona o objeto indicado pela tecla 1-9 e imprime seu material no console.
static void handleObjectSelection(int key)
{
    if (key < GLFW_KEY_1 || key > GLFW_KEY_9) return;
    int idx = key - GLFW_KEY_1;
    if (idx >= (int)g_app.objects.size()) return;

    g_app.selected = idx;
    g_app.rotAxis  = RotationAxis::None;

    const SceneObject& obj = g_app.objects[g_app.selected];
    std::cout << "\nSelecionado: " << obj.name << "\n";
    if (!obj.submeshes.empty()) {
        const Material& m = obj.submeshes[0].mat;
        std::cout << "  Material: ka=" << m.ka << "  kd=" << m.kd
                  << "  ks=" << m.ks << "  shininess=" << m.shininess << "\n";
    }
}

/// @brief Alterna o estado enabled das luzes key/fill/back via teclas [, ] e \.
static void handleLightToggle(int key)
{
    struct { int k; int idx; const char* name; } bindings[] = {
        {GLFW_KEY_LEFT_BRACKET,  0, "Key light"},
        {GLFW_KEY_RIGHT_BRACKET, 1, "Fill light"},
        {GLFW_KEY_BACKSLASH,     2, "Back light"},
    };
    for (auto& b : bindings) {
        if (key == b.k && b.idx < (int)g_app.lights.size()) {
            g_app.lights[b.idx].enabled = !g_app.lights[b.idx].enabled;
            std::cout << b.name << ": " << (g_app.lights[b.idx].enabled ? "ON" : "OFF") << "\n";
            uploadLights(g_app.shader, g_app.lights);
            return;
        }
    }
}

/// @brief Define o eixo de rotacao automatica do objeto selecionado (X/Y/Z) ou para (P).
static void handleRotationAxis(int key)
{
    switch (key) {
        case GLFW_KEY_X: g_app.rotAxis = RotationAxis::X;    break;
        case GLFW_KEY_Y: g_app.rotAxis = RotationAxis::Y;    break;
        case GLFW_KEY_Z: g_app.rotAxis = RotationAxis::Z;    break;
        case GLFW_KEY_P: g_app.rotAxis = RotationAxis::None; break;
        default: break;
    }
}

/**
 * @brief Ajusta a intensidade de todas as luzes com as teclas I (aumenta) e O (diminui).
 * @param key  Codigo GLFW da tecla.
 * @param held true se a tecla esta pressionada ou sendo repetida.
 */
static void handleLightIntensity(int key, bool held)
{
    if (!held) return;
    float delta = (key == GLFW_KEY_I) ? 0.1f : (key == GLFW_KEY_O) ? -0.1f : 0.0f;
    if (delta == 0.0f) return;

    for (auto& l : g_app.lights)
        l.intensity = glm::clamp(l.intensity + delta, 0.0f, 5.0f);
    uploadLights(g_app.shader, g_app.lights);
    if (!g_app.lights.empty())
        std::cout << "Intensidade luz: " << g_app.lights[0].intensity << "\n";
}

/**
 * @brief Gerencia acoes de trajetoria do objeto: F (adiciona ponto), T (toggle),
 *        C (limpa) e M (salva em arquivo).
 * @param obj Objeto selecionado cujas trajetoria e posicao serao manipuladas.
 * @param key Codigo GLFW da tecla pressionada.
 */
static void handleTrajectory(SceneObject& obj, int key)
{
    switch (key) {
        case GLFW_KEY_F:
            obj.trajectory.addPoint(obj.position);
            std::cout << obj.name << ": " << obj.trajectory.points.size() << " ponto(s)\n";
            break;
        case GLFW_KEY_T:
            if (!obj.trajectory.hasPath())
                std::cout << obj.name << ": adicione >= 2 pontos com F primeiro\n";
            else {
                obj.trajectory.active = !obj.trajectory.active;
                std::cout << obj.name << ": trajetoria "
                          << (obj.trajectory.active ? "ATIVADA" : "DESATIVADA") << "\n";
            }
            break;
        case GLFW_KEY_C:
            obj.trajectory.clear();
            std::cout << obj.name << ": trajetoria limpa\n";
            break;
        case GLFW_KEY_M:
            if (obj.trajectory.points.empty())
                std::cout << obj.name << ": trajetoria vazia, nada a salvar\n";
            else {
                obj.trajectory.saveToFile(obj.trajectoryFile);
                std::cout << obj.name << ": trajetoria salva em " << obj.trajectoryFile << "\n";
            }
            break;
        default: break;
    }
}

/**
 * @brief Aplica translacao (setas/PgUp/PgDn) e escala (numpad +/-) ao objeto
 *        apenas quando a trajetoria esta inativa.
 * @param obj  Objeto selecionado a ser transformado.
 * @param key  Codigo GLFW da tecla.
 * @param held true se a tecla esta pressionada ou sendo repetida.
 */
static void handleTransformWhenStatic(SceneObject& obj, int key, bool held)
{
    if (!held || obj.trajectory.active) return;

    struct { int k; glm::vec3 posDelta; glm::vec3 scaleDelta; } bindings[] = {
        {GLFW_KEY_UP,          { 0,  STEP_T,  0}, {0,0,0}},
        {GLFW_KEY_DOWN,        { 0, -STEP_T,  0}, {0,0,0}},
        {GLFW_KEY_LEFT,        {-STEP_T, 0,   0}, {0,0,0}},
        {GLFW_KEY_RIGHT,       { STEP_T, 0,   0}, {0,0,0}},
        {GLFW_KEY_PAGE_UP,     { 0, 0,  STEP_T}, {0,0,0}},
        {GLFW_KEY_PAGE_DOWN,   { 0, 0, -STEP_T}, {0,0,0}},
        {GLFW_KEY_KP_ADD,      {0,0,0}, { STEP_S,  STEP_S,  STEP_S}},
        {GLFW_KEY_KP_SUBTRACT, {0,0,0}, {-STEP_S, -STEP_S, -STEP_S}},
    };

    for (auto& b : bindings) {
        if (key != b.k) continue;
        obj.position += b.posDelta;
        obj.scale     = glm::max(obj.scale + b.scaleDelta, glm::vec3(MIN_S));
        return;
    }
}

// ---------- callbacks publicos ----------

/**
 * @brief Callback de teclado GLFW — despacha para os handlers de selecao,
 *        luzes, rotacao, trajetoria e transformacao do objeto selecionado.
 * @param win    Janela GLFW que gerou o evento.
 * @param key    Codigo GLFW da tecla.
 * @param action GLFW_PRESS, GLFW_RELEASE ou GLFW_REPEAT.
 */
void keyCallback(GLFWwindow* win, int key, int /*sc*/, int action, int /*mods*/)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(win, GL_TRUE);
        return;
    }

    bool pressed = (action == GLFW_PRESS);
    bool held    = (action == GLFW_PRESS || action == GLFW_REPEAT);

    if (pressed) {
        handleObjectSelection(key);
        handleLightToggle(key);
        handleRotationAxis(key);
    }

    handleLightIntensity(key, held);

    if (g_app.selected >= (int)g_app.objects.size()) return;
    SceneObject& obj = g_app.objects[g_app.selected];

    if (pressed) handleTrajectory(obj, key);
    handleTransformWhenStatic(obj, key, held);
}

/// @brief Callback de movimento de mouse GLFW — repassa coordenadas para a camera.
/// @param x Posicao horizontal do cursor em pixels.
/// @param y Posicao vertical do cursor em pixels.
void mouseCallback(GLFWwindow* /*w*/, double x, double y)
{
    g_app.camera.processMouse(x, y);
}

/// @brief Callback de scroll do mouse GLFW — repassa deslocamento vertical para o FOV da camera.
/// @param y Deslocamento vertical do scroll (positivo = zoom in).
void scrollCallback(GLFWwindow* /*w*/, double /*x*/, double y)
{
    g_app.camera.processScroll(y);
}
