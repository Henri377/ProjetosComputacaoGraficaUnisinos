#include <iostream>
#include <fstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "InputHandler.h"
#include "SceneParser.h"
#include "OBJLoader.h"
#include "Shader.h"

const GLuint WIN_W = 1200, WIN_H = 800;

// ---- uniform locations agrupados -------------------------------------------

/// @brief Cache de localizacoes de uniforms do shader para evitar lookups repetidos por frame.
struct Uniforms {
    GLint model, view, proj, normal, camPos;
    GLint ka, kd, ks, shine;
};

// ---- funcoes de inicializacao -----------------------------------------------

/**
 * @brief Determina o caminho do arquivo de cena a partir dos argumentos ou de paths padrao.
 * @param argc Numero de argumentos da linha de comando.
 * @param argv Vetor de argumentos; argv[1], se presente, e usado diretamente.
 * @return Caminho para o arquivo de cena a ser carregado.
 */
static std::string resolveScenePath(int argc, char** argv)
{
    if (argc > 1) return argv[1];
    if (std::ifstream("../assets/scene.txt").is_open()) return "../assets/scene.txt";
    return "../../assets/scene.txt";
}

/**
 * @brief Inicializa GLFW, cria a janela OpenGL 4.5 e registra os callbacks de entrada.
 * @return Ponteiro para a janela criada, ou nullptr em caso de falha.
 */
static GLFWwindow* initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(WIN_W, WIN_H, "Visualizador de Cenas 3D", nullptr, nullptr);
    if (!win) { std::cerr << "GLFW: falha ao criar janela\n"; return nullptr; }

    glfwMakeContextCurrent(win);
    glfwSetKeyCallback(win,       keyCallback);
    glfwSetCursorPosCallback(win, mouseCallback);
    glfwSetScrollCallback(win,    scrollCallback);
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    return win;
}

/**
 * @brief Transfere os parametros de camera do SceneDef para o objeto Camera global.
 * @param scene Descricao da cena com posicao, yaw, pitch, fov, near e far da camera.
 */
static void applySceneCamera(const SceneDef& scene)
{
    g_app.camera.position = scene.camPos;
    g_app.camera.yaw      = scene.camYaw;
    g_app.camera.pitch    = scene.camPitch;
    g_app.camera.fov      = scene.camFov;
    g_app.camera.nearP    = scene.camNear;
    g_app.camera.farP     = scene.camFar;
    g_app.camera.init();
}

/**
 * @brief Constroi um SceneObject a partir de uma ObjSpec, resolvendo o caminho da trajetoria.
 * @param spec        Especificacao do objeto lida do arquivo de cena.
 * @param index       Indice do objeto na lista (usado para gerar nome de trajetoria padrao).
 * @param sceneDir    Diretorio do arquivo de cena (base para paths relativos).
 * @param fallbackTex Textura padrao usada quando o MTL nao define map_Kd (nao aplicado aqui).
 * @return SceneObject preenchido com nome, posicao, escala, rotacao e trajetoria.
 */
static SceneObject buildObject(const SceneDef::ObjSpec& spec, int index,
                               const std::string& sceneDir, const std::string& fallbackTex)
{
    SceneObject obj;
    obj.name             = spec.name;
    obj.position         = spec.position;
    obj.scale            = spec.scale;
    obj.baseRot          = spec.rotation;
    obj.trajectory.speed = spec.trajSpeed;
    obj.trajectoryFile   = spec.trajectoryFile.empty()
        ? resolvePath(sceneDir, "../assets/trajetoria_" + std::to_string(index) + ".txt")
        : resolvePath(sceneDir, spec.trajectoryFile);
    return obj;
}

/**
 * @brief Carrega todos os objetos e luzes da cena para o estado global da aplicacao.
 * @param scene    Descricao da cena com luzes e especificacoes de objetos.
 * @param sceneDir Diretorio do arquivo de cena usado para resolver paths relativos.
 * @return true se ao menos um objeto foi carregado com sucesso.
 */
static bool loadObjects(const SceneDef& scene, const std::string& sceneDir)
{
    std::string fallback = resolvePath(sceneDir, "tex/pixelWall.png");

    g_app.lights = scene.lights;
    if (g_app.lights.empty()) {
        g_app.lights.push_back(Light{});
        std::cout << "Aviso: nenhuma luz no arquivo de cena; usando padrao.\n";
    }

    for (int i = 0; i < (int)scene.objects.size(); ++i) {
        SceneObject obj = buildObject(scene.objects[i], i, sceneDir, fallback);
        std::string objPath = resolvePath(sceneDir, scene.objects[i].file);

        std::cout << "OBJ: " << objPath << "\n";
        if (!loadOBJ(objPath, obj.submeshes, fallback)) {
            std::cerr << "  ERRO: falha ao carregar " << objPath << "\n";
            continue;
        }
        if (obj.trajectory.loadFromFile(obj.trajectoryFile)) {
            std::cout << "  Trajetoria: " << obj.trajectory.points.size() << " pontos\n";
            obj.trajectory.active = true;
        }
        g_app.objects.push_back(std::move(obj));
    }
    return !g_app.objects.empty();
}

/**
 * @brief Consulta e armazena as locations de todos os uniforms do shader principal.
 * @param shader Identificador OpenGL do programa de shader.
 * @return Struct Uniforms preenchida com as locations.
 */
static Uniforms cacheUniforms(GLuint shader)
{
    auto loc = [&](const char* name) { return glGetUniformLocation(shader, name); };
    return { loc("model"), loc("view"), loc("projection"), loc("normalMatrix"),
             loc("cameraPos"), loc("ka"), loc("kd"), loc("ks"), loc("shininess") };
}

/// @brief Imprime no console a lista de teclas de controle disponiveis.
static void printControls()
{
    std::cout << "\n=== Controles ===\n"
              << "1-9      Seleciona objeto (" << g_app.objects.size() << " carregado(s))\n"
              << "Setas    Move XY  |  PgUp/Dn: move Z  (trajetoria inativa)\n"
              << "+/-      Escala (numpad)\n"
              << "X/Y/Z    Rotacao automatica  |  P: para\n"
              << "F/T/C/M  Trajetoria: adiciona/toggle/limpa/salva\n"
              << "I/O      Intensidade das luzes\n"
              << "[/]/\\   Toggle luzes key/fill/back\n"
              << "WASD     Camera  |  Mouse: orienta  |  Scroll: zoom\n"
              << "ESC      Sai\n\n";
}

// ---- funcoes do loop --------------------------------------------------------

/**
 * @brief Aplica rotacao automatica continua ao objeto com base no eixo selecionado.
 * @param obj  Objeto cujo campo rotation sera atualizado.
 * @param axis Eixo de rotacao (X, Y, Z ou None).
 * @param time Tempo absoluto em segundos (glfwGetTime), usado para calcular o angulo.
 */
static void applyRotation(SceneObject& obj, RotationAxis axis, float time)
{
    float angle = time * 50.0f;
    switch (axis) {
        case RotationAxis::X: obj.rotation = {angle, 0, 0}; break;
        case RotationAxis::Y: obj.rotation = {0, angle, 0}; break;
        case RotationAxis::Z: obj.rotation = {0, 0, angle}; break;
        default: break;
    }
}

/**
 * @brief Calcula as matrizes de modelo e normal do objeto e renderiza todas as suas submalhas.
 * @param obj Objeto a ser desenhado com suas transformacoes e submalhas.
 * @param u   Cache de locations de uniforms do shader.
 */
static void drawSceneObject(const SceneObject& obj, const Uniforms& u)
{
    glm::vec3 rot   = obj.baseRot + obj.rotation;
    glm::mat4 model = glm::scale(
                      glm::rotate(
                      glm::rotate(
                      glm::rotate(
                      glm::translate(glm::mat4(1.0f), obj.position),
                          glm::radians(rot.x), {1, 0, 0}),
                          glm::radians(rot.y), {0, 1, 0}),
                          glm::radians(rot.z), {0, 0, 1}),
                          obj.scale);

    glm::mat3 normMat = glm::transpose(glm::inverse(glm::mat3(model)));
    glUniformMatrix4fv(u.model,  1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix3fv(u.normal, 1, GL_FALSE, glm::value_ptr(normMat));

    for (const auto& sm : obj.submeshes) {
        glUniform1f(u.ka,    sm.mat.ka);
        glUniform1f(u.kd,    sm.mat.kd);
        glUniform1f(u.ks,    sm.mat.ks);
        glUniform1f(u.shine, sm.mat.shininess);
        sm.draw();
    }
}

/**
 * @brief Executa um frame completo: atualiza tempo, camera, trajetórias, rotacoes e renderiza.
 * @param win Janela GLFW usada para polling de eventos e swap de buffers.
 * @param u   Cache de locations de uniforms do shader.
 */
static void updateFrame(GLFWwindow* win, const Uniforms& u)
{
    float now       = (float)glfwGetTime();
    g_app.delta     = now - g_app.lastFrame;
    g_app.lastFrame = now;

    g_app.camera.processKeyboard(win, g_app.delta);
    glfwPollEvents();

    for (auto& obj : g_app.objects) {
        if (obj.trajectory.active && obj.trajectory.hasPath()) {
            obj.trajectory.update(g_app.delta);
            obj.position = obj.trajectory.currentPos();
        }
    }

    if (g_app.selected < (int)g_app.objects.size())
        applyRotation(g_app.objects[g_app.selected], g_app.rotAxis, now);

    glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int fw, fh;
    glfwGetFramebufferSize(win, &fw, &fh);
    float aspect = (fh > 0) ? (float)fw / fh : 1.0f;
    glUniformMatrix4fv(u.view, 1, GL_FALSE, glm::value_ptr(g_app.camera.viewMatrix()));
    glUniformMatrix4fv(u.proj, 1, GL_FALSE, glm::value_ptr(g_app.camera.projMatrix(aspect)));
    glUniform3fv(u.camPos, 1, glm::value_ptr(g_app.camera.position));

    for (const auto& obj : g_app.objects)
        drawSceneObject(obj, u);

    glfwSwapBuffers(win);
}

// ---- entry point ------------------------------------------------------------

/**
 * @brief Ponto de entrada da aplicacao — inicializa janela, OpenGL, shaders,
 *        carrega a cena e executa o loop de renderizacao ate o usuario fechar.
 * @param argc Numero de argumentos da linha de comando.
 * @param argv Vetor de argumentos; argv[1] pode ser o caminho do arquivo de cena.
 * @return 0 em caso de sucesso, -1 em caso de falha de inicializacao.
 */
int main(int argc, char** argv)
{
    std::string sceneFile = resolveScenePath(argc, argv);

    GLFWwindow* win = initWindow();
    if (!win) return -1;

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD: falha na inicializacao\n"; return -1;
    }
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n"
              << "OpenGL:   " << glGetString(GL_VERSION)  << "\n\n";

    glViewport(0, 0, WIN_W, WIN_H);
    glEnable(GL_DEPTH_TEST);

    g_app.shader = compileShaders();
    glUseProgram(g_app.shader);
    glUniform1i(glGetUniformLocation(g_app.shader, "texBuff"), 0);

    std::cout << "Carregando cena: " << sceneFile << "\n";
    SceneDef scene = parseScene(sceneFile);
    applySceneCamera(scene);

    if (!loadObjects(scene, dirOf(sceneFile))) {
        std::cerr << "Nenhum objeto carregado. Verifique " << sceneFile << "\n";
        return -1;
    }

    uploadLights(g_app.shader, g_app.lights);
    Uniforms uni = cacheUniforms(g_app.shader);
    printControls();

    while (!glfwWindowShouldClose(win))
        updateFrame(win, uni);

    for (auto& obj : g_app.objects) obj.release();
    glfwTerminate();
    return 0;
}
