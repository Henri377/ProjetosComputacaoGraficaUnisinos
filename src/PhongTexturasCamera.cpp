/* PhongTexturasCamera.cpp - Phong + Texturas com camera livre
 * Baseado em PhongTexturas.cpp e adiciona:
 * - camera FPS (WASD)
 * - mouse look (yaw/pitch)
 * - zoom via scroll (FOV)
 * - velocidade com deltaTime
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
    fragPos = vec3(model * vec4(position, 1.0));
    scaledNormal = vec3(model * vec4(normal, 1.0));
    fragTexc = texc;
})";

const GLchar *fragmentShaderSource = R"(
#version 450
in vec2 fragTexc;
in vec3 fragPos;
in vec3 scaledNormal;
out vec4 color;
uniform sampler2D texBuff;
uniform float ka;
uniform float kd;
uniform float ks;
uniform float q;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 cameraPos;
void main()
{
    vec3 objectColor = texture(texBuff, fragTexc).rgb;

    vec3 ambient = ka * lightColor;

    vec3 N = normalize(scaledNormal);
    vec3 L = normalize(lightPos - fragPos);
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = kd * diff * lightColor;

    vec3 V = normalize(cameraPos - fragPos);
    vec3 R = normalize(reflect(-L, N));
    float spec = max(dot(R, V), 0.0);
    spec = pow(spec, q);
    vec3 specular = ks * spec * lightColor;

    vec3 result = (ambient + diffuse) * objectColor + specular;
    color = vec4(result, 1.0);
})";

bool rotateX = false, rotateY = false, rotateZ = false;

// Variaveis globais da camera
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

bool firstMouse = true;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
float yaw = -90.0f;
float pitch = 0.0f;
float fov = 45.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct Material
{
    float  ka = 0.2f;
    float  kd = 0.7f;
    float  ks = 0.5f;
    float  q  = 32.0f;
    string textureName;
};

struct Mesh
{
    GLuint VAO = 0;
    GLuint textureID = 0;
    int nVertices = 0;
    glm::vec3 position{0.0f};
    glm::vec3 scale{1.0f};
    glm::vec3 baseRotation{0.0f};
    glm::vec3 rotation{0.0f};
    float ka = 0.2f;
    float kd = 0.7f;
    float ks = 0.5f;
    float q  = 32.0f;
};

int selectedMesh = 0;
Mesh suzanne;
Mesh cube;
Mesh *meshes[2] = {&suzanne, &cube};
const float SCALE_STEP     = 0.1f;
const float SCALE_MIN      = 0.1f;
const float TRANSLATE_STEP = 0.1f;

/** Processa teclas de transformacao da malha selecionada. */
void handleMeshTransformKeys(Mesh &active, int key, int action);
/** Atualiza apenas os uniforms relacionados a camera (view/projection/pos). */
void updateCameraUniforms(GLuint shaderID, GLint viewLoc, GLint projLoc);
/** Aplica rotacao automatica (X/Y/Z) na malha selecionada. */
void updateActiveMeshRotation();

/** Callback de teclado para selecao, rotacao e transformacao de malhas. */
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
/** Callback de mouse para atualizar yaw/pitch da camera. */
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
/** Callback de scroll para zoom in/out alterando o FOV. */
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
/** Processa input continuo (WASD da camera por frame). */
void processInput(GLFWwindow *window);
/** Compila e linka vertex/fragment shader e retorna o programa. */
int setupShader();

/** Retorna o diretorio base de um caminho de arquivo. */
string directoryOf(const string &filePath)
{
    size_t pos = filePath.find_last_of("/\\");
    if (pos == string::npos)
        return "";
    return filePath.substr(0, pos + 1);
}

string resolvePath(const string &baseDir, const string &relativePath)
{
    if (relativePath.empty())
        return "";
    if (relativePath.find(':') != string::npos || relativePath[0] == '/' ||
        (relativePath.size() > 1 && relativePath[1] == ':'))
        return relativePath;
    return baseDir + relativePath;
}

Material loadMaterialFromMTL(const string &mtlPath)
{
    Material mat;
    ifstream mtlFile(mtlPath);
    if (!mtlFile.is_open())
    {
        cerr << "Aviso: nao foi possivel abrir MTL: " << mtlPath << " (usando defaults)" << endl;
        return mat;
    }

    string line;
    while (getline(mtlFile, line))
    {
        istringstream ss(line);
        string keyword;
        ss >> keyword;

        if (keyword == "Ka")
        {
            float r; ss >> r; mat.ka = r;
        }
        else if (keyword == "Kd")
        {
            float r; ss >> r; mat.kd = r;
        }
        else if (keyword == "Ks")
        {
            float r; ss >> r; mat.ks = r;
        }
        else if (keyword == "Ns")
        {
            ss >> mat.q;
        }
        else if (keyword == "map_Kd")
        {
            ss >> mat.textureName;
        }
    }
    return mat;
}

bool loadTexturedOBJ(const string &objPath, GLuint &outVAO, int &nVertices, Material &outMaterial)
{
    vector<glm::vec3> vertices;
    vector<glm::vec2> texCoords;
    vector<glm::vec3> normals;
    vector<GLfloat>   vBuffer;

    string objDir = directoryOf(objPath);
    string mtlFileName;

    ifstream objFile(objPath);
    if (!objFile.is_open())
    {
        cerr << "Erro ao tentar ler o arquivo " << objPath << endl;
        return false;
    }

    string line;
    while (getline(objFile, line))
    {
        istringstream ssline(line);
        string word;
        ssline >> word;

        if (word == "mtllib")
        {
            ssline >> mtlFileName;
        }
        else if (word == "v")
        {
            glm::vec3 v;
            ssline >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        }
        else if (word == "vt")
        {
            glm::vec2 vt;
            ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        }
        else if (word == "vn")
        {
            glm::vec3 n;
            ssline >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (word == "f")
        {
            while (ssline >> word)
            {
                int vi = 0, ti = 0, ni = 0;
                istringstream ss(word);
                string index;

                if (getline(ss, index, '/'))
                    vi = !index.empty() ? stoi(index) - 1 : 0;
                if (getline(ss, index, '/'))
                    ti = !index.empty() ? stoi(index) - 1 : 0;
                if (getline(ss, index))
                    ni = !index.empty() ? stoi(index) - 1 : 0;

                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);

                if (!texCoords.empty() && ti >= 0 && ti < (int)texCoords.size())
                {
                    vBuffer.push_back(texCoords[ti].s);
                    vBuffer.push_back(1.0f - texCoords[ti].t);
                }
                else
                {
                    vBuffer.push_back(0.0f);
                    vBuffer.push_back(0.0f);
                }

                if (!normals.empty() && ni >= 0 && ni < (int)normals.size())
                {
                    vBuffer.push_back(normals[ni].x);
                    vBuffer.push_back(normals[ni].y);
                    vBuffer.push_back(normals[ni].z);
                }
                else
                {
                    vBuffer.push_back(0.0f);
                    vBuffer.push_back(1.0f);
                    vBuffer.push_back(0.0f);
                }
            }
        }
    }
    objFile.close();

    outMaterial = Material{};
    if (!mtlFileName.empty())
    {
        string mtlPath = resolvePath(objDir, mtlFileName);
        outMaterial = loadMaterialFromMTL(mtlPath);
        if (!outMaterial.textureName.empty())
            outMaterial.textureName = resolvePath(objDir, outMaterial.textureName);
    }

    if (vBuffer.empty())
    {
        cerr << "Nenhuma face encontrada em " << objPath << endl;
        return false;
    }

    const GLsizei stride = 8 * sizeof(GLfloat);
    GLuint VBO = 0;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &outVAO);
    glBindVertexArray(outVAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid *)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = static_cast<int>(vBuffer.size() / 8);
    cout << "OBJ carregado: " << objPath << " (" << nVertices << " vertices)" << endl;
    return true;
}

GLuint loadTexture(const string &filePath, int &width, int &height)
{
    GLuint texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int nrChannels = 0;
    unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);
    if (!data)
    {
        cerr << "Falha ao carregar textura: " << filePath << endl;
        glDeleteTextures(1, &texID);
        return 0;
    }

    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}

bool loadMesh(Mesh &mesh, const string &objPath)
{
    Material mat;
    if (!loadTexturedOBJ(objPath, mesh.VAO, mesh.nVertices, mat))
        return false;

    mesh.ka = mat.ka;
    mesh.kd = mat.kd;
    mesh.ks = mat.ks;
    mesh.q  = mat.q;

    string texturePath = mat.textureName;
    if (texturePath.empty())
        texturePath = "../assets/tex/pixelWall.png";

    int w = 0, h = 0;
    mesh.textureID = loadTexture(texturePath, w, h);
    return mesh.textureID != 0;
}

int main()
{
    glfwInit();

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Phong + Texturas + Camera", nullptr, nullptr);
    if (!window)
        return -1;

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    cout << "Renderer: " << glGetString(GL_RENDERER) << endl;
    cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    GLuint shaderID = setupShader();
    glUseProgram(shaderID);
    glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);

    if (!loadMesh(suzanne, "../assets/Modelos3D/Suzanne.obj"))
        return -1;
    suzanne.position     = glm::vec3(-0.8f, 0.0f, 0.0f);
    suzanne.scale        = glm::vec3(0.6f);
    suzanne.baseRotation = glm::vec3(0.0f, 180.0f, 0.0f);

    if (!loadMesh(cube, "../assets/Modelos3D/Cube.obj"))
        return -1;
    cube.position     = glm::vec3(0.8f, 0.0f, 0.0f);
    cube.scale        = glm::vec3(0.6f);
    cube.baseRotation = glm::vec3(0.0f, 180.0f, 0.0f);

    glm::vec3 lightPos   = glm::vec3(2.0f, 3.0f, 5.0f);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);

    glUniform3fv(glGetUniformLocation(shaderID, "lightPos"),   1, glm::value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(shaderID, "lightColor"), 1, glm::value_ptr(lightColor));

    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint viewLoc  = glGetUniformLocation(shaderID, "view");
    GLint projLoc  = glGetUniformLocation(shaderID, "projection");
    GLint kaLoc    = glGetUniformLocation(shaderID, "ka");
    GLint kdLoc    = glGetUniformLocation(shaderID, "kd");
    GLint ksLoc    = glGetUniformLocation(shaderID, "ks");
    GLint qLoc     = glGetUniformLocation(shaderID, "q");

    glEnable(GL_DEPTH_TEST);

    auto drawMesh = [&](const Mesh &mesh)
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, mesh.position);
        glm::vec3 rot = mesh.baseRotation + mesh.rotation;
        model = glm::rotate(model, glm::radians(rot.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rot.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, mesh.scale);

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1f(kaLoc, mesh.ka);
        glUniform1f(kdLoc, mesh.kd);
        glUniform1f(ksLoc, mesh.ks);
        glUniform1f(qLoc,  mesh.q);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mesh.textureID);
        glBindVertexArray(mesh.VAO);
        glDrawArrays(GL_TRIANGLES, 0, mesh.nVertices);
        glBindVertexArray(0);
    };

    while (!glfwWindowShouldClose(window))
    {
        // Atualiza deltaTime para manter velocidade constante.
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        updateCameraUniforms(shaderID, viewLoc, projLoc);
        updateActiveMeshRotation();

        drawMesh(suzanne);
        drawMesh(cube);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &suzanne.VAO);
    glDeleteVertexArrays(1, &cube.VAO);
    glDeleteTextures(1, &suzanne.textureID);
    glDeleteTextures(1, &cube.textureID);
    glfwTerminate();
    return 0;
}

/** Processa WASD da camera e atalho ESC de saida. */
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    float cameraSpeed = 2.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

/** Converte movimento do mouse em yaw/pitch e atualiza eixos da camera. */
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    (void)window;

    if (firstMouse)
    {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos);
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    float sensitivity = 0.05f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);

    glm::vec3 right = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0f, 1.0f, 0.0f)));
    cameraUp = glm::normalize(glm::cross(right, cameraFront));
}

/** Atualiza campo de visao (zoom) no intervalo [1, 45]. */
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    (void)window;
    (void)xoffset;

    if (fov >= 1.0f && fov <= 45.0f)
        fov -= static_cast<float>(yoffset);
    if (fov <= 1.0f)
        fov = 1.0f;
    if (fov >= 45.0f)
        fov = 45.0f;
}

/** Atualiza uniforms de view/projection/cameraPos a cada frame. */
void updateCameraUniforms(GLuint shaderID, GLint viewLoc, GLint projLoc)
{
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    int fbWidth = 0;
    int fbHeight = 0;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &fbWidth, &fbHeight);
    float aspect = (fbHeight > 0) ? (static_cast<float>(fbWidth) / static_cast<float>(fbHeight)) : 1.0f;
    glm::mat4 projection = glm::perspective(glm::radians(fov), aspect, 0.1f, 100.0f);

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(shaderID, "cameraPos"), 1, glm::value_ptr(cameraPos));
}

/** Aplica rotacao automatica no eixo ativo da malha selecionada. */
void updateActiveMeshRotation()
{
    float angle = static_cast<float>(glfwGetTime()) * 50.0f;
    Mesh &active = (selectedMesh == 0) ? suzanne : cube;

    if (rotateX)
        active.rotation = glm::vec3(angle, 0.0f, 0.0f);
    else if (rotateY)
        active.rotation = glm::vec3(0.0f, angle, 0.0f);
    else if (rotateZ)
        active.rotation = glm::vec3(0.0f, 0.0f, angle);
}

/** Trata entradas de translacao e escala da malha selecionada. */
void handleMeshTransformKeys(Mesh &active, int key, int action)
{
    // Movimentacao de objeto remapeada para setas e PageUp/PageDown.
    if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.position.y += TRANSLATE_STEP;
    if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.position.y -= TRANSLATE_STEP;
    if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.position.x -= TRANSLATE_STEP;
    if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.position.x += TRANSLATE_STEP;
    if (key == GLFW_KEY_PAGE_UP && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.position.z += TRANSLATE_STEP;
    if (key == GLFW_KEY_PAGE_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.position.z -= TRANSLATE_STEP;

    if (key == GLFW_KEY_KP_ADD && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.scale += glm::vec3(SCALE_STEP);

    if (key == GLFW_KEY_KP_SUBTRACT && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        active.scale -= glm::vec3(SCALE_STEP);
        active.scale.x = max(active.scale.x, SCALE_MIN);
        active.scale.y = max(active.scale.y, SCALE_MIN);
        active.scale.z = max(active.scale.z, SCALE_MIN);
    }

    if (key == GLFW_KEY_I && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.scale.x += SCALE_STEP;
    if (key == GLFW_KEY_K && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.scale.x = max(active.scale.x - SCALE_STEP, SCALE_MIN);

    if (key == GLFW_KEY_O && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.scale.y += SCALE_STEP;
    if (key == GLFW_KEY_L && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.scale.y = max(active.scale.y - SCALE_STEP, SCALE_MIN);

    if (key == GLFW_KEY_P && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.scale.z += SCALE_STEP;
    if (key == GLFW_KEY_SEMICOLON && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.scale.z = max(active.scale.z - SCALE_STEP, SCALE_MIN);
}

/** Processa selecao de malha, eixo de rotacao e transformacoes por teclado. */
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    (void)window;
    (void)scancode;
    (void)mode;

    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
        selectedMesh = 0;
    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
        selectedMesh = 1;

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        rotateX = true;
        rotateY = rotateZ = false;
    }
    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        rotateY = true;
        rotateX = rotateZ = false;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        rotateZ = true;
        rotateX = rotateY = false;
    }

    Mesh &active = (selectedMesh == 0) ? suzanne : cube;
    handleMeshTransformKeys(active, key, action);
}

/** Compila shaders, faz link e retorna o programa OpenGL. */
int setupShader()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}
