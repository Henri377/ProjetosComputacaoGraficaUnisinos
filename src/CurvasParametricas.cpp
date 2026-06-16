/* CurvasParametricas.cpp - Trajetórias paramétricas para objetos 3D
 * Baseado em PhongTexturasCamera.cpp
 *
 * Controles:
 *  1 / 2        - Seleciona objeto (Suzanne / Cubo)
 *  F            - Adiciona posição atual do objeto à sua trajetória
 *  T            - Ativa/desativa animação de trajetória do objeto selecionado
 *  C            - Limpa trajetória do objeto selecionado
 *  M            - Salva trajetória em arquivo (assets/trajetoria_N.txt)
 *  Setas        - Move objeto selecionado (X/Y), apenas com trajetória inativa
 *  PgUp/PgDn    - Move objeto selecionado (Z), apenas com trajetória inativa
 *  +/-          - Escala uniforme
 *  X / Y / Z    - Rotação automática no eixo escolhido
 *  WASD         - Move câmera (FPS)
 *  Mouse        - Orienta câmera
 *  Scroll       - Zoom (altera FOV)
 *  ESC          - Sai
 *
 * Trajetórias são carregadas de arquivo ao iniciar (se existirem):
 *   assets/trajetoria_0.txt  (Suzanne)
 *   assets/trajetoria_1.txt  (Cubo)
 * Formato: uma posição "x y z" por linha; linhas com '#' são ignoradas.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

const GLuint WIDTH = 1000, HEIGHT = 1000;

// ---- Shaders (iguais ao PhongTexturasCamera) ----------------------------

const GLchar *vertexShaderSource = R"(
#version 450
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texc;
layout (location = 2) in vec3 normal;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec2 fragTexc;
out vec3 fragPos;
out vec3 scaledNormal;
void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
    fragPos     = vec3(model * vec4(position, 1.0));
    scaledNormal = vec3(model * vec4(normal, 1.0));
    fragTexc    = texc;
})";

const GLchar *fragmentShaderSource = R"(
#version 450
in vec2 fragTexc;
in vec3 fragPos;
in vec3 scaledNormal;
out vec4 color;
uniform sampler2D texBuff;
uniform float ka, kd, ks, q;
uniform vec3 lightPos, lightColor, cameraPos;
void main()
{
    vec3 objectColor = texture(texBuff, fragTexc).rgb;
    vec3 ambient  = ka * lightColor;
    vec3 N = normalize(scaledNormal);
    vec3 L = normalize(lightPos - fragPos);
    vec3 diffuse  = kd * max(dot(N, L), 0.0) * lightColor;
    vec3 V = normalize(cameraPos - fragPos);
    vec3 R = normalize(reflect(-L, N));
    vec3 specular = ks * pow(max(dot(R, V), 0.0), q) * lightColor;
    color = vec4((ambient + diffuse) * objectColor + specular, 1.0);
})";

// ---- Estado global -------------------------------------------------------

bool  rotateX = false, rotateY = false, rotateZ = false;
float deltaTime = 0.0f, lastFrame = 0.0f;

// ---- Camera --------------------------------------------------------------

class Camera
{
public:
    Camera()
        : position(0.0f, 0.0f, 5.0f), front(0.0f, 0.0f, -1.0f),
          up(0.0f, 1.0f, 0.0f), yaw(-90.0f), pitch(0.0f), fov(45.0f),
          firstMouse(true), lastX(WIDTH / 2.0f), lastY(HEIGHT / 2.0f),
          speed(2.5f), sensitivity(0.05f)
    {}

    void ProcessKeyboard(GLFWwindow *window, float dt)
    {
        float s = speed * dt;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) position += s * front;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) position -= s * front;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            position -= glm::normalize(glm::cross(front, up)) * s;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            position += glm::normalize(glm::cross(front, up)) * s;
    }

    void ProcessMouseMovement(double xpos, double ypos)
    {
        if (firstMouse) { lastX = (float)xpos; lastY = (float)ypos; firstMouse = false; }
        float xo = ((float)xpos - lastX) * sensitivity;
        float yo = (lastY - (float)ypos)  * sensitivity;
        lastX = (float)xpos; lastY = (float)ypos;
        yaw  += xo;
        pitch = glm::clamp(pitch + yo, -89.0f, 89.0f);
        glm::vec3 f;
        f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.y = sin(glm::radians(pitch));
        f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(f);
        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0,1,0)));
        up = glm::normalize(glm::cross(right, front));
    }

    void ProcessMouseScroll(double yo)
    {
        fov = glm::clamp(fov - (float)yo, 1.0f, 45.0f);
    }

    glm::mat4 GetViewMatrix()       const { return glm::lookAt(position, position + front, up); }
    glm::mat4 GetProjectionMatrix(float aspect) const
        { return glm::perspective(glm::radians(fov), aspect, 0.1f, 100.0f); }
    const glm::vec3 &GetPosition()  const { return position; }

private:
    glm::vec3 position, front, up;
    float yaw, pitch, fov;
    bool  firstMouse;
    float lastX, lastY, speed, sensitivity;
};

// ---- Textura 2D ----------------------------------------------------------

class Texture2D
{
public:
    bool LoadFromFile(const string &path)
    {
        Release();
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        int w, h, ch;
        unsigned char *data = stbi_load(path.c_str(), &w, &h, &ch, 0);
        if (!data) { cerr << "Falha ao carregar textura: " << path << endl; Release(); return false; }
        GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_2D, 0);
        return true;
    }
    void Bind(GLenum unit = GL_TEXTURE0) const
        { glActiveTexture(unit); glBindTexture(GL_TEXTURE_2D, id); }
    void Release() { if (id) { glDeleteTextures(1, &id); id = 0; } }
private:
    GLuint id = 0;
};

// ---- Trajetória paramétrica cíclica -------------------------------------
//
// Cada segmento vai de points[currentIdx] a points[(currentIdx+1) % N].
// O parâmetro t ∈ [0,1) percorre o segmento; ao atingir 1, avança para o
// próximo ponto de forma cíclica (ao sair do último, retorna ao primeiro).
// A posição é calculada por interpolação linear (LERP):
//   pos = mix(points[i], points[i+1], t)

struct Trajectory
{
    vector<glm::vec3> points;
    int   currentIdx = 0;
    float t          = 0.0f;  // parâmetro no segmento atual, em [0, 1)
    float speed      = 0.8f;  // unidades de mundo por segundo
    bool  active     = false;

    bool hasPath() const { return (int)points.size() >= 2; }

    // Posição interpolada no instante atual.
    glm::vec3 currentPosition() const
    {
        if (points.empty())        return glm::vec3(0.0f);
        if ((int)points.size() == 1) return points[0];
        int next = (currentIdx + 1) % (int)points.size();
        return glm::mix(points[currentIdx], points[next], t);
    }

    // Avança a trajetória em dt segundos.
    void update(float dt)
    {
        if (!active || !hasPath()) return;

        int   next   = (currentIdx + 1) % (int)points.size();
        float segLen = glm::length(points[next] - points[currentIdx]);

        // Quanto avançar em t para percorrer (speed * dt) unidades neste segmento.
        float step = (segLen > 0.0001f) ? (speed * dt / segLen) : 1.0f;

        t += step;
        if (t >= 1.0f)
        {
            // Chegou ao próximo ponto — avança e reinicia t (ciclo automático).
            t          = 0.0f;
            currentIdx = next;   // quando next == 0 o ciclo recomeça
        }
    }

    void addPoint(const glm::vec3 &p)
    {
        points.push_back(p);
        cout << "  Ponto " << points.size() << " adicionado: ("
             << p.x << ", " << p.y << ", " << p.z << ")" << endl;
    }

    void clear()
    {
        points.clear();
        currentIdx = 0;
        t          = 0.0f;
        active     = false;
        cout << "  Trajetoria limpa." << endl;
    }
};

// Lê pontos de um arquivo texto ("x y z" por linha; '#' ignora a linha).
bool loadTrajectoryFromFile(Trajectory &traj, const string &path)
{
    ifstream f(path);
    if (!f.is_open()) return false;
    string line;
    while (getline(f, line))
    {
        if (line.empty() || line[0] == '#') continue;
        istringstream ss(line);
        glm::vec3 p;
        if (ss >> p.x >> p.y >> p.z) traj.points.push_back(p);
    }
    if (!traj.points.empty())
    {
        cout << "Trajetoria carregada de " << path
             << " (" << traj.points.size() << " pontos)" << endl;
        return true;
    }
    return false;
}

// Salva pontos em arquivo texto.
void saveTrajectoryToFile(const Trajectory &traj, const string &path)
{
    ofstream f(path);
    f << "# Trajetoria - " << traj.points.size() << " pontos\n";
    for (const auto &p : traj.points)
        f << p.x << " " << p.y << " " << p.z << "\n";
    cout << "Trajetoria salva em " << path
         << " (" << traj.points.size() << " pontos)" << endl;
}

// ---- Tipos de cena -------------------------------------------------------

struct Material { float ka=0.2f, kd=0.7f, ks=0.5f, q=32.0f; string textureName; };

struct Mesh
{
    GLuint    VAO = 0;
    Texture2D texture;
    int       nVertices  = 0;
    glm::vec3 position{0.0f};
    glm::vec3 scale{1.0f};
    glm::vec3 baseRotation{0.0f};
    glm::vec3 rotation{0.0f};
    float     ka=0.2f, kd=0.7f, ks=0.5f, q=32.0f;
    Trajectory trajectory;   // trajetória paramétrica deste objeto
};

// ---- Globais de cena -----------------------------------------------------

Camera gCamera;
int    selectedMesh = 0;
Mesh   suzanne, cube;
Mesh  *meshes[2] = {&suzanne, &cube};

const float SCALE_STEP     = 0.1f;
const float SCALE_MIN      = 0.1f;
const float TRANSLATE_STEP = 0.1f;

const string TRAJ_FILES[2] = {
    "../assets/trajetoria_0.txt",
    "../assets/trajetoria_1.txt"
};

// ---- Protótipos ----------------------------------------------------------

void key_callback(GLFWwindow*, int, int, int, int);
void mouse_callback(GLFWwindow*, double, double);
void scroll_callback(GLFWwindow*, double, double);
void processInput(GLFWwindow*);
void updateCameraUniforms(GLFWwindow*, GLuint, GLint, GLint);
void updateActiveMeshRotation();
void handleMeshTransformKeys(Mesh&, int, int);
int  setupShader();

// ---- Utilitários de caminho ----------------------------------------------

static string directoryOf(const string &p)
{
    size_t pos = p.find_last_of("/\\");
    return (pos == string::npos) ? "" : p.substr(0, pos + 1);
}
static string resolvePath(const string &base, const string &rel)
{
    if (rel.empty()) return "";
    if (rel.find(':') != string::npos || rel[0] == '/') return rel;
    return base + rel;
}

// ---- Carregamento de OBJ e MTL ------------------------------------------

Material loadMaterialFromMTL(const string &path)
{
    Material mat;
    ifstream f(path);
    if (!f.is_open()) { cerr << "Aviso: MTL nao encontrado: " << path << endl; return mat; }
    string line;
    while (getline(f, line))
    {
        istringstream ss(line); string kw; ss >> kw;
        if      (kw == "Ka")     { float v; ss >> v; mat.ka = v; }
        else if (kw == "Kd")     { float v; ss >> v; mat.kd = v; }
        else if (kw == "Ks")     { float v; ss >> v; mat.ks = v; }
        else if (kw == "Ns")     { ss >> mat.q; }
        else if (kw == "map_Kd") { ss >> mat.textureName; }
    }
    return mat;
}

bool loadTexturedOBJ(const string &objPath, GLuint &outVAO, int &nVerts, Material &outMat)
{
    vector<glm::vec3> verts; vector<glm::vec2> uvs; vector<glm::vec3> norms;
    vector<GLfloat>   buf;
    string objDir = directoryOf(objPath), mtlFile;

    ifstream f(objPath);
    if (!f.is_open()) { cerr << "Erro ao abrir: " << objPath << endl; return false; }

    string line;
    while (getline(f, line))
    {
        istringstream ss(line); string w; ss >> w;
        if      (w == "mtllib") { ss >> mtlFile; }
        else if (w == "v")  { glm::vec3 v; ss>>v.x>>v.y>>v.z; verts.push_back(v); }
        else if (w == "vt") { glm::vec2 u; ss>>u.s>>u.t;      uvs.push_back(u); }
        else if (w == "vn") { glm::vec3 n; ss>>n.x>>n.y>>n.z; norms.push_back(n); }
        else if (w == "f")
        {
            while (ss >> w)
            {
                int vi=0, ti=0, ni=0;
                istringstream sw(w); string idx;
                if (getline(sw,idx,'/')) vi = idx.empty() ? 0 : stoi(idx)-1;
                if (getline(sw,idx,'/')) ti = idx.empty() ? 0 : stoi(idx)-1;
                if (getline(sw,idx))    ni = idx.empty() ? 0 : stoi(idx)-1;

                buf.push_back(verts[vi].x); buf.push_back(verts[vi].y); buf.push_back(verts[vi].z);

                if (!uvs.empty() && ti>=0 && ti<(int)uvs.size())
                    { buf.push_back(uvs[ti].s); buf.push_back(1.0f-uvs[ti].t); }
                else
                    { buf.push_back(0); buf.push_back(0); }

                if (!norms.empty() && ni>=0 && ni<(int)norms.size())
                    { buf.push_back(norms[ni].x); buf.push_back(norms[ni].y); buf.push_back(norms[ni].z); }
                else
                    { buf.push_back(0); buf.push_back(1); buf.push_back(0); }
            }
        }
    }
    f.close();
    if (buf.empty()) { cerr << "Nenhuma face em " << objPath << endl; return false; }

    outMat = Material{};
    if (!mtlFile.empty())
    {
        string mp = resolvePath(objDir, mtlFile);
        outMat = loadMaterialFromMTL(mp);
        if (!outMat.textureName.empty())
            outMat.textureName = resolvePath(objDir, outMat.textureName);
    }

    const GLsizei stride = 8 * sizeof(GLfloat);
    GLuint VBO = 0;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, buf.size()*sizeof(GLfloat), buf.data(), GL_STATIC_DRAW);
    glGenVertexArrays(1, &outVAO);
    glBindVertexArray(outVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(3*sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(5*sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVerts = (int)(buf.size() / 8);
    cout << "OBJ carregado: " << objPath << " (" << nVerts << " vertices)" << endl;
    return true;
}

bool loadMesh(Mesh &mesh, const string &objPath)
{
    Material mat;
    if (!loadTexturedOBJ(objPath, mesh.VAO, mesh.nVertices, mat)) return false;
    mesh.ka = mat.ka; mesh.kd = mat.kd; mesh.ks = mat.ks; mesh.q = mat.q;
    string tex = mat.textureName.empty() ? "../assets/tex/pixelWall.png" : mat.textureName;
    return mesh.texture.LoadFromFile(tex);
}

// ---- main ----------------------------------------------------------------

int main()
{
    glfwInit();
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT,
        "Curvas Parametricas - Trajetorias", nullptr, nullptr);
    if (!window) return -1;
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window,    key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        { cout << "Falha ao inicializar GLAD" << endl; return -1; }

    cout << "Renderer: "       << glGetString(GL_RENDERER) << endl;
    cout << "OpenGL version: " << glGetString(GL_VERSION)  << endl;

    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);

    GLuint shaderID = setupShader();
    glUseProgram(shaderID);
    glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);

    if (!loadMesh(suzanne, "../assets/Modelos3D/Suzanne.obj")) return -1;
    suzanne.position     = glm::vec3(-0.8f, 0.0f, 0.0f);
    suzanne.scale        = glm::vec3(0.6f);
    suzanne.baseRotation = glm::vec3(0.0f, 180.0f, 0.0f);

    if (!loadMesh(cube, "../assets/Modelos3D/Cube.obj")) return -1;
    cube.position     = glm::vec3(0.8f, 0.0f, 0.0f);
    cube.scale        = glm::vec3(0.6f);
    cube.baseRotation = glm::vec3(0.0f, 180.0f, 0.0f);

    // Tenta carregar trajetórias salvas de sessões anteriores.
    for (int i = 0; i < 2; ++i)
        loadTrajectoryFromFile(meshes[i]->trajectory, TRAJ_FILES[i]);

    glm::vec3 lightPos(2.0f, 3.0f, 5.0f), lightColor(1.0f);
    glUniform3fv(glGetUniformLocation(shaderID,"lightPos"),   1, glm::value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(shaderID,"lightColor"), 1, glm::value_ptr(lightColor));

    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint viewLoc  = glGetUniformLocation(shaderID, "view");
    GLint projLoc  = glGetUniformLocation(shaderID, "projection");
    GLint kaLoc    = glGetUniformLocation(shaderID, "ka");
    GLint kdLoc    = glGetUniformLocation(shaderID, "kd");
    GLint ksLoc    = glGetUniformLocation(shaderID, "ks");
    GLint qLoc     = glGetUniformLocation(shaderID, "q");

    glEnable(GL_DEPTH_TEST);

    auto drawMesh = [&](const Mesh &m)
    {
        glm::mat4 model(1.0f);
        model = glm::translate(model, m.position);
        glm::vec3 rot = m.baseRotation + m.rotation;
        model = glm::rotate(model, glm::radians(rot.x), glm::vec3(1,0,0));
        model = glm::rotate(model, glm::radians(rot.y), glm::vec3(0,1,0));
        model = glm::rotate(model, glm::radians(rot.z), glm::vec3(0,0,1));
        model = glm::scale(model, m.scale);

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1f(kaLoc, m.ka); glUniform1f(kdLoc, m.kd);
        glUniform1f(ksLoc, m.ks); glUniform1f(qLoc,  m.q);
        m.texture.Bind(GL_TEXTURE0);
        glBindVertexArray(m.VAO);
        glDrawArrays(GL_TRIANGLES, 0, m.nVertices);
        glBindVertexArray(0);
    };

    cout << "\n=== Controles ===" << endl;
    cout << "1/2     - Seleciona objeto (Suzanne / Cubo)" << endl;
    cout << "F       - Adiciona posicao atual a trajetoria do objeto" << endl;
    cout << "T       - Ativa/desativa trajetoria do objeto selecionado" << endl;
    cout << "C       - Limpa trajetoria do objeto selecionado" << endl;
    cout << "M       - Salva trajetoria em arquivo" << endl;
    cout << "Setas   - Move objeto (X/Y)  |  PgUp/Dn - Move objeto (Z)" << endl;
    cout << "WASD    - Camera  |  Mouse - Orienta  |  Scroll - Zoom" << endl;
    cout << "ESC     - Sai\n" << endl;

    // ---- Game loop -------------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        float now = (float)glfwGetTime();
        deltaTime = now - lastFrame;
        lastFrame = now;

        processInput(window);
        glfwPollEvents();

        // Atualiza trajetórias e reposiciona cada objeto.
        for (int i = 0; i < 2; ++i)
        {
            Mesh &m = *meshes[i];
            if (m.trajectory.active && m.trajectory.hasPath())
            {
                m.trajectory.update(deltaTime);
                m.position = m.trajectory.currentPosition();
            }
        }

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        updateCameraUniforms(window, shaderID, viewLoc, projLoc);
        updateActiveMeshRotation();

        drawMesh(suzanne);
        drawMesh(cube);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &suzanne.VAO);
    glDeleteVertexArrays(1, &cube.VAO);
    suzanne.texture.Release();
    cube.texture.Release();
    glfwTerminate();
    return 0;
}

// ---- Callbacks -----------------------------------------------------------

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    gCamera.ProcessKeyboard(window, deltaTime);
}

void mouse_callback(GLFWwindow *window, double x, double y)
    { (void)window; gCamera.ProcessMouseMovement(x, y); }

void scroll_callback(GLFWwindow *window, double xo, double yo)
    { (void)window; (void)xo; gCamera.ProcessMouseScroll(yo); }

void updateCameraUniforms(GLFWwindow *window, GLuint shaderID, GLint viewLoc, GLint projLoc)
{
    int w=0, h=0;
    glfwGetFramebufferSize(window, &w, &h);
    float aspect = (h > 0) ? (float)w / (float)h : 1.0f;

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(gCamera.GetViewMatrix()));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE,
        glm::value_ptr(gCamera.GetProjectionMatrix(aspect)));
    glUniform3fv(glGetUniformLocation(shaderID, "cameraPos"),
        1, glm::value_ptr(gCamera.GetPosition()));
}

void updateActiveMeshRotation()
{
    float angle  = (float)glfwGetTime() * 50.0f;
    Mesh &active = *meshes[selectedMesh];
    if      (rotateX) active.rotation = glm::vec3(angle, 0, 0);
    else if (rotateY) active.rotation = glm::vec3(0, angle, 0);
    else if (rotateZ) active.rotation = glm::vec3(0, 0, angle);
}

// Move o objeto manualmente apenas quando sua trajetória não está ativa.
void handleMeshTransformKeys(Mesh &m, int key, int action)
{
    if (m.trajectory.active) return;  // trajetória controla a posição

    bool held = (action == GLFW_PRESS || action == GLFW_REPEAT);
    if (key == GLFW_KEY_UP        && held) m.position.y += TRANSLATE_STEP;
    if (key == GLFW_KEY_DOWN      && held) m.position.y -= TRANSLATE_STEP;
    if (key == GLFW_KEY_LEFT      && held) m.position.x -= TRANSLATE_STEP;
    if (key == GLFW_KEY_RIGHT     && held) m.position.x += TRANSLATE_STEP;
    if (key == GLFW_KEY_PAGE_UP   && held) m.position.z += TRANSLATE_STEP;
    if (key == GLFW_KEY_PAGE_DOWN && held) m.position.z -= TRANSLATE_STEP;

    if (key == GLFW_KEY_KP_ADD      && held) m.scale += glm::vec3(SCALE_STEP);
    if (key == GLFW_KEY_KP_SUBTRACT && held)
    {
        m.scale -= glm::vec3(SCALE_STEP);
        m.scale  = glm::max(m.scale, glm::vec3(SCALE_MIN));
    }
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    (void)window; (void)scancode; (void)mode;

    // Seleciona objeto ativo.
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) selectedMesh = 0;
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) selectedMesh = 1;

    // Eixo de rotação automática.
    if (key == GLFW_KEY_X && action == GLFW_PRESS) { rotateX=true;  rotateY=rotateZ=false; }
    if (key == GLFW_KEY_Y && action == GLFW_PRESS) { rotateY=true;  rotateX=rotateZ=false; }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS) { rotateZ=true;  rotateX=rotateY=false; }

    Mesh &active = *meshes[selectedMesh];

    // ---- Controles de trajetória ----------------------------------------

    // F: registra a posição atual do objeto como novo ponto de controle.
    if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        cout << "Objeto " << selectedMesh << " - ";
        active.trajectory.addPoint(active.position);
        cout << "  Total de pontos: " << active.trajectory.points.size() << endl;
    }

    // T: liga/desliga animação de trajetória.
    if (key == GLFW_KEY_T && action == GLFW_PRESS)
    {
        if (!active.trajectory.hasPath())
        {
            cout << "Objeto " << selectedMesh
                 << ": adicione pelo menos 2 pontos com F antes de ativar a trajetoria." << endl;
        }
        else
        {
            active.trajectory.active = !active.trajectory.active;
            cout << "Objeto " << selectedMesh << " - trajetoria "
                 << (active.trajectory.active ? "ATIVADA" : "DESATIVADA") << endl;
        }
    }

    // C: apaga todos os pontos e para a animação.
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        cout << "Objeto " << selectedMesh << " - ";
        active.trajectory.clear();
    }

    // M: salva os pontos em arquivo para reutilização futura.
    if (key == GLFW_KEY_M && action == GLFW_PRESS)
    {
        if (active.trajectory.points.empty())
            cout << "Objeto " << selectedMesh << ": trajetoria vazia, nada a salvar." << endl;
        else
            saveTrajectoryToFile(active.trajectory, TRAJ_FILES[selectedMesh]);
    }

    // Movimentação manual (bloqueada quando trajetória está ativa).
    handleMeshTransformKeys(active, key, action);
}

// ---- Compilação de shaders -----------------------------------------------

int setupShader()
{
    auto compile = [](GLenum type, const GLchar *src) -> GLuint
    {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, NULL);
        glCompileShader(s);
        GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            GLchar log[512]; glGetShaderInfoLog(s, 512, NULL, log);
            cerr << "Shader error:\n" << log << endl;
        }
        return s;
    };

    GLuint vs = compile(GL_VERTEX_SHADER,   vertexShaderSource);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { GLchar log[512]; glGetProgramInfoLog(prog,512,NULL,log); cerr<<log<<endl; }

    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}
