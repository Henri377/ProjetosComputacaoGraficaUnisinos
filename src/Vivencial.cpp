/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para as disciplinas de Processamento Gráfico/Computação Gráfica - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 07/03/2025
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 1000, HEIGHT = 1000;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar *vertexShaderSource = "#version 450\n"
                                   "layout (location = 0) in vec3 position;\n"
                                   "layout (location = 1) in vec3 color;\n"
                                   "uniform mat4 model;\n"
                                   "out vec4 finalColor;\n"
                                   "void main()\n"
                                   "{\n"
                                   //...pode ter mais linhas de código aqui!
                                   "gl_Position = model * vec4(position, 1.0);\n"
                                   "finalColor = vec4(color, 1.0);\n"
                                   "}\0";

// Códifo fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar *fragmentShaderSource = "#version 450\n"
                                     "in vec4 finalColor;\n"
                                     "out vec4 color;\n"
                                     "void main()\n"
                                     "{\n"
                                     "color = finalColor;\n"
                                     "}\n\0";

bool rotateX = false, rotateY = false, rotateZ = false;

struct Mesh
{
    GLuint VAO;
    int nVertices;
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation; // degrees (x, y, z)
};

int loadSimpleOBJ(string filePATH, int &nVertices)
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<GLfloat> vBuffer;
    glm::vec3 color = glm::vec3(1.0, 0.0, 0.0);

    std::ifstream arqEntrada(filePATH.c_str());
    if (!arqEntrada.is_open())
    {
        std::cerr << "Erro ao tentar ler o arquivo " << filePATH << std::endl;
        return -1;
    }

    std::string line;
    while (std::getline(arqEntrada, line))
    {
        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "v")
        {
            glm::vec3 vertice;
            ssline >> vertice.x >> vertice.y >> vertice.z;
            vertices.push_back(vertice);
        }
        else if (word == "vt")
        {
            glm::vec2 vt;
            ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        }
        else if (word == "vn")
        {
            glm::vec3 normal;
            ssline >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        else if (word == "f")
        {
            while (ssline >> word)
            {
                int vi = 0, ti = 0, ni = 0;
                std::istringstream ss(word);
                std::string index;

                if (std::getline(ss, index, '/'))
                    vi = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index, '/'))
                    ti = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index))
                    ni = !index.empty() ? std::stoi(index) - 1 : 0;

                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);
                vBuffer.push_back(color.r);
                vBuffer.push_back(color.g);
                vBuffer.push_back(color.b);
            }
        }
    }

    arqEntrada.close();

    std::cout << "Gerando o buffer de geometria..." << std::endl;
    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = vBuffer.size() / 6; // x, y, z, r, g, b (valores atualmente armazenados por vértice)

    return VAO;
}

int selectedMesh = 0;
Mesh suzanne;
Mesh cube;
Mesh *meshes[2] = {&suzanne, &cube};
const float SCALE_STEP = 0.1f;
const float SCALE_MIN = 0.1f;
const float TRANSLATE_STEP = 0.1f;

int main()
{
    // Init GLFW
    glfwInit();

    // Create window
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Two OBJs", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    // Init GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    // Print version info
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *version = glGetString(GL_VERSION);
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version: " << version << endl;

    // Viewport
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Shader
    GLuint shaderID = setupShader();
    glUseProgram(shaderID);

    // --- Load meshes ---
    suzanne.VAO = loadSimpleOBJ("../assets/Modelos3D/Suzanne.obj", suzanne.nVertices);
    suzanne.position = glm::vec3(-0.5f, 0.0f, 0.0f);
    suzanne.scale = glm::vec3(0.2f, 0.2f, 0.2f);
    suzanne.rotation = glm::vec3(0.0f, 0.0f, 0.0f);

    cube.VAO = loadSimpleOBJ("../assets/Modelos3D/Suzanne.obj", cube.nVertices);
    cube.position = glm::vec3(0.5f, 0.0f, 0.0f);
    cube.scale = glm::vec3(0.2f, 0.2f, 0.2f);
    cube.rotation = glm::vec3(0.0f, 0.0f, 0.0f);

    // Uniform location
    GLint modelLoc = glGetUniformLocation(shaderID, "model");

    glEnable(GL_DEPTH_TEST);

    // --- Helper lambda to build and send model matrix ---
    auto drawMesh = [&](const Mesh &mesh)
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, mesh.position);
        model = glm::rotate(model, glm::radians(mesh.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(mesh.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(mesh.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, mesh.scale);

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(mesh.VAO);
        glDrawArrays(GL_TRIANGLES, 0, mesh.nVertices);
        glBindVertexArray(0);
    };

    // --- Game loop ---
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Clear
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Rotate only the selected mesh
        float angle = (GLfloat)glfwGetTime() * 50.0f;
        Mesh &active = (selectedMesh == 0) ? suzanne : cube;

        if (rotateX)
            active.rotation = glm::vec3(angle, 0.0f, 0.0f);
        else if (rotateY)
            active.rotation = glm::vec3(0.0f, angle, 0.0f);
        else if (rotateZ)
            active.rotation = glm::vec3(0.0f, 0.0f, angle);

        // Draw both
        drawMesh(suzanne);
        drawMesh(cube);

        glfwSwapBuffers(window);
    }

    // Cleanup
    glDeleteVertexArrays(1, &suzanne.VAO);
    glDeleteVertexArrays(1, &cube.VAO);
    glfwTerminate();
    return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // --- Select active mesh ---
    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
        selectedMesh = 0;
    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
        selectedMesh = 1;

    // --- Rotation axis ---
    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        rotateX = true;
        rotateY = false;
        rotateZ = false;
    }
    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        rotateX = false;
        rotateY = true;
        rotateZ = false;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        rotateX = false;
        rotateY = false;
        rotateZ = true;
    }

    // --- Active mesh reference ---
    Mesh &active = (selectedMesh == 0) ? suzanne : cube;

    // --- Translation (WASD + Q/E) ---
    if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.position.y += TRANSLATE_STEP;

    if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.position.y -= TRANSLATE_STEP;

    if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.position.x -= TRANSLATE_STEP;

    if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.position.x += TRANSLATE_STEP;

    if (key == GLFW_KEY_Q && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.position.z += TRANSLATE_STEP;

    if (key == GLFW_KEY_E && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.position.z -= TRANSLATE_STEP;

    // --- Scale uniform (numpad + / -) ---
    if (key == GLFW_KEY_KP_ADD && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.scale += glm::vec3(SCALE_STEP);

    if (key == GLFW_KEY_KP_SUBTRACT && (action == GLFW_PRESS || action == GLFW_REPEAT))

    {
        active.scale -= glm::vec3(SCALE_STEP);
        active.scale.x = max(active.scale.x, SCALE_MIN);
        active.scale.y = max(active.scale.y, SCALE_MIN);
        active.scale.z = max(active.scale.z, SCALE_MIN);
    }

    // --- Scale per axis ---
    // X
    if (key == GLFW_KEY_I && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.scale.x += SCALE_STEP;
    if (key == GLFW_KEY_K && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.scale.x = max(active.scale.x - SCALE_STEP, SCALE_MIN);

    // Y
    if (key == GLFW_KEY_O && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.scale.y += SCALE_STEP;
    if (key == GLFW_KEY_L && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.scale.y = max(active.scale.y - SCALE_STEP, SCALE_MIN);

    // Z
    if (key == GLFW_KEY_P && (action == GLFW_PRESS || action == GLFW_REPEAT))
        active.scale.z += SCALE_STEP;
    if (key == GLFW_KEY_SEMICOLON && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        active.scale.z = max(active.scale.z - SCALE_STEP, SCALE_MIN);
    }
}

// Esta função está basntante hardcoded - objetivo é compilar e "buildar" um programa de
//  shader simples e único neste exemplo de código
//  O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
//  fragmentShader source no iniçio deste arquivo
//  A função retorna o identificador do programa de shader
int setupShader()
{
    // Vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // Checando erros de compilação (exibição via log no terminal)
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // Checando erros de compilação (exibição via log no terminal)
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    // Linkando os shaders e criando o identificador do programa de shader
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // Checando por erros de linkagem
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}
