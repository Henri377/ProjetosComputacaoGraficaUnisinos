/* VisualizadorCenas.cpp - Visualizador de cenas 3D com arquivo de configuracao
 *
 * Uso: VisualizadorCenas [arquivo_de_cena.txt]
 *      (padrao: ../assets/scene.txt)
 *
 * Controles:
 *  1-9      - Seleciona objeto
 *  Setas    - Move objeto (X/Y), apenas com trajetoria inativa
 *  PgUp/Dn  - Move objeto (Z)
 *  +/-      - Escala uniforme (numpad)
 *  X/Y/Z    - Liga rotacao automatica no eixo
 *  P        - Para rotacao automatica
 *  F        - Adiciona posicao atual como ponto de trajetoria
 *  T        - Ativa/desativa trajetoria
 *  C        - Limpa trajetoria
 *  M        - Salva trajetoria em arquivo
 *  I/O      - Aumenta/diminui intensidade da(s) luz(es)
 *  WASD     - Move camera (FPS)
 *  Mouse    - Orienta camera
 *  Scroll   - Zoom (FOV)
 *  ESC      - Sai
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <algorithm>
#include <cmath>

using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// ---- Dimensoes da janela ----------------------------------------------------

const GLuint WIN_W = 1200, WIN_H = 800;
const int    MAX_LIGHTS = 4;

// ---- Shaders ----------------------------------------------------------------

const char *VERT_SRC = R"glsl(
#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNorm;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

out vec3 fragPos;
out vec2 fragUV;
out vec3 fragNorm;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    gl_Position   = projection * view * worldPos;
    fragPos  = vec3(worldPos);
    fragNorm = normalMatrix * aNorm;
    fragUV   = aUV;
}
)glsl";

// Phong com suporte a ate MAX_LIGHTS fontes de luz parametrizaveis.
const char *FRAG_SRC = R"glsl(
#version 450 core
in vec3 fragPos;
in vec2 fragUV;
in vec3 fragNorm;
out vec4 outColor;

uniform sampler2D texBuff;
uniform vec3 cameraPos;

uniform float ka;
uniform float kd;
uniform float ks;
uniform float shininess;

uniform int   numLights;
uniform vec3  lightPos[4];
uniform vec3  lightColor[4];
uniform float lightIntensity[4];

void main() {
    vec3 texColor = texture(texBuff, fragUV).rgb;
    vec3 N = normalize(fragNorm);
    vec3 V = normalize(cameraPos - fragPos);

    vec3 result = vec3(0.0);
    for (int i = 0; i < numLights && i < 4; i++) {
        vec3  L  = normalize(lightPos[i] - fragPos);
        vec3  R  = reflect(-L, N);
        vec3  lc = lightColor[i] * lightIntensity[i];

        vec3 ambient  = ka * lc;
        vec3 diffuse  = kd * max(dot(N, L), 0.0) * lc;
        vec3 specular = ks * pow(max(dot(R, V), 0.0), shininess) * lc;

        result += (ambient + diffuse) * texColor + specular;
    }
    outColor = vec4(result, 1.0);
}
)glsl";

// ---- Camera (FPS) -----------------------------------------------------------

class Camera {
public:
    glm::vec3 position{0, 0, 5};
    float yaw      = -90.0f;
    float pitch    =   0.0f;
    float fov      =  45.0f;
    float nearP    =   0.1f;
    float farP     = 100.0f;
    float speed    =   2.5f;
    float mouseSens=   0.05f;

    void init() { updateVectors(); }

    void processKeyboard(GLFWwindow *w, float dt) {
        float s = speed * dt;
        if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) position += s * front;
        if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) position -= s * front;
        if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS)
            position -= glm::normalize(glm::cross(front, up)) * s;
        if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS)
            position += glm::normalize(glm::cross(front, up)) * s;
    }

    void processMouse(double xpos, double ypos) {
        if (firstMouse) { lastX = (float)xpos; lastY = (float)ypos; firstMouse = false; }
        float dx = ((float)xpos - lastX) * mouseSens;
        float dy = (lastY - (float)ypos)  * mouseSens;
        lastX = (float)xpos; lastY = (float)ypos;
        yaw  += dx;
        pitch = glm::clamp(pitch + dy, -89.0f, 89.0f);
        updateVectors();
    }

    void processScroll(double dy) { fov = glm::clamp(fov - (float)dy, 1.0f, 89.0f); }

    glm::mat4 viewMatrix() const { return glm::lookAt(position, position + front, up); }
    glm::mat4 projMatrix(float aspect) const {
        return glm::perspective(glm::radians(fov), aspect, nearP, farP);
    }

private:
    glm::vec3 front{0, 0, -1}, up{0, 1, 0};
    bool  firstMouse = true;
    float lastX = WIN_W / 2.0f, lastY = WIN_H / 2.0f;

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

// ---- Textura 2D -------------------------------------------------------------

struct Texture2D {
    GLuint id = 0;

    bool load(const string &path) {
        release();
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        int w, h, ch;
        unsigned char *data = stbi_load(path.c_str(), &w, &h, &ch, 0);
        if (!data) {
            cerr << "  Aviso: textura nao encontrada: " << path << "\n";
            // Textura 1x1 cinza como fallback
            unsigned char gray[3] = {128, 128, 128};
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, gray);
        } else {
            GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        return true;
    }

    void bind(GLenum unit = GL_TEXTURE0) const {
        glActiveTexture(unit);
        glBindTexture(GL_TEXTURE_2D, id);
    }

    void release() {
        if (id) { glDeleteTextures(1, &id); id = 0; }
    }
};

// ---- Material ---------------------------------------------------------------

struct Material {
    float  ka        = 0.2f;
    float  kd        = 0.7f;
    float  ks        = 0.5f;
    float  shininess = 32.0f;
    string texPath;
};

// ---- Trajetoria parametrica - Bezier cubico (Catmull-Rom) -------------------
//
// Os pontos de controle do arquivo sao usados como knots de uma spline
// Catmull-Rom, que e convertida para Bezier cubico por segmento.
// Formula: B(t) = (1-t)^3*b0 + 3(1-t)^2*t*b1 + 3(1-t)*t^2*b2 + t^3*b3
// onde b0..b3 sao derivados dos pontos vizinhos (tangentes automaticas).

struct Trajectory {
    vector<glm::vec3> points;
    int   idx    = 0;
    float t      = 0.0f;
    float speed  = 0.8f;  // unidades/segundo
    bool  active = false;

    bool hasPath() const { return (int)points.size() >= 2; }

    // Avalia Bezier cubico para o segmento atual usando Catmull-Rom.
    glm::vec3 currentPos() const {
        if (points.empty())          return {};
        if ((int)points.size() == 1) return points[0];
        int N  = (int)points.size();
        // Quatro pontos vizinhos para Catmull-Rom ciclico
        glm::vec3 p0 = points[((idx - 1) + N) % N];
        glm::vec3 p1 = points[idx];
        glm::vec3 p2 = points[(idx + 1) % N];
        glm::vec3 p3 = points[(idx + 2) % N];
        // Pontos de controle Bezier derivados das tangentes Catmull-Rom
        glm::vec3 b0 = p1;
        glm::vec3 b1 = p1 + (p2 - p0) / 6.0f;
        glm::vec3 b2 = p2 - (p3 - p1) / 6.0f;
        glm::vec3 b3 = p2;
        float u = 1.0f - t;
        return u*u*u*b0 + 3.0f*u*u*t*b1 + 3.0f*u*t*t*b2 + t*t*t*b3;
    }

    // Estima o comprimento do segmento Bezier atual por amostragem.
    float segmentLength() const {
        const int SAMPLES = 10;
        float len = 0.0f;
        float tOld = t;
        // Salva t, amostra o segmento inteiro
        const_cast<Trajectory*>(this)->t = 0.0f;
        glm::vec3 prev = currentPos();
        for (int i = 1; i <= SAMPLES; ++i) {
            const_cast<Trajectory*>(this)->t = (float)i / SAMPLES;
            glm::vec3 cur = currentPos();
            len += glm::length(cur - prev);
            prev = cur;
        }
        const_cast<Trajectory*>(this)->t = tOld;
        return (len > 1e-4f) ? len : 1.0f;
    }

    void update(float dt) {
        if (!active || !hasPath()) return;
        float len  = segmentLength();
        float step = speed * dt / len;
        t += step;
        if (t >= 1.0f) {
            t   = 0.0f;
            idx = (idx + 1) % (int)points.size();
        }
    }

    void addPoint(const glm::vec3 &p) { points.push_back(p); }

    void clear() { points.clear(); idx = 0; t = 0.0f; active = false; }

    bool loadFromFile(const string &path) {
        ifstream f(path);
        if (!f.is_open()) return false;
        string line;
        while (getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            istringstream ss(line); glm::vec3 p;
            if (ss >> p.x >> p.y >> p.z) points.push_back(p);
        }
        return !points.empty();
    }

    void saveToFile(const string &path) const {
        ofstream f(path);
        f << "# trajetoria - " << points.size() << " pontos\n";
        for (auto &p : points)
            f << p.x << " " << p.y << " " << p.z << "\n";
    }
};

// ---- SubMesh (um grupo/material de um OBJ) ----------------------------------

struct SubMesh {
    GLuint    VAO    = 0;
    GLuint    VBO    = 0;
    int       nVerts = 0;
    Material  mat;
    Texture2D tex;

    void draw() const {
        tex.bind(GL_TEXTURE0);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, nVerts);
        glBindVertexArray(0);
    }

    void release() {
        if (VAO) { glDeleteVertexArrays(1, &VAO); VAO = 0; }
        if (VBO) { glDeleteBuffers(1, &VBO);      VBO = 0; }
        tex.release();
    }
};

// ---- Objeto de cena ---------------------------------------------------------

struct SceneObject {
    string          name;
    vector<SubMesh> submeshes;
    glm::vec3       position{0};
    glm::vec3       scale{1};
    glm::vec3       baseRot{0};   // rotacao inicial (do arquivo de cena)
    glm::vec3       rotation{0};  // rotacao automatica acumulada
    Trajectory      trajectory;
    string          trajectoryFile;

    void release() {
        for (auto &sm : submeshes) sm.release();
        submeshes.clear();
    }
};

// ---- Fonte de luz -----------------------------------------------------------

struct Light {
    glm::vec3 position{2, 3, 5};
    glm::vec3 color{1, 1, 1};
    float     intensity = 1.0f;
    bool      enabled   = true;
};

// ---- Utilitarios de caminho -------------------------------------------------

static string dirOf(const string &p) {
    size_t pos = p.find_last_of("/\\");
    return (pos == string::npos) ? "" : p.substr(0, pos + 1);
}

static string resolve(const string &base, const string &rel) {
    if (rel.empty()) return "";
    if (rel.find(':') != string::npos || rel[0] == '/' || rel[0] == '\\') return rel;
    return base + rel;
}

static string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

// ---- Carregamento de OBJ com suporte a multiplos grupos/materiais -----------
//
// Um SubMesh e criado por grupo de material (usemtl).
// Faces sao triangularizadas por fan triangulation.

bool loadOBJ(const string &objPath, vector<SubMesh> &out, const string &fallbackTex) {
    vector<glm::vec3> vPos;
    vector<glm::vec2> vUV;
    vector<glm::vec3> vNorm;
    map<string, Material> mats;

    string objDir  = dirOf(objPath);
    string mtlFile;

    struct Group { string matName; vector<GLfloat> buf; };
    vector<Group> groups;
    groups.push_back({"default", {}});

    ifstream f(objPath);
    if (!f.is_open()) { cerr << "OBJ nao encontrado: " << objPath << "\n"; return false; }

    string line;
    while (getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        // Remove carriage return (Windows line endings)
        if (!line.empty() && line.back() == '\r') line.pop_back();
        istringstream ss(line); string kw; ss >> kw;

        if      (kw == "mtllib") { ss >> mtlFile; }
        else if (kw == "v")  { glm::vec3 v; ss>>v.x>>v.y>>v.z; vPos.push_back(v); }
        else if (kw == "vt") { glm::vec2 u; ss>>u.s>>u.t;      vUV.push_back(u); }
        else if (kw == "vn") { glm::vec3 n; ss>>n.x>>n.y>>n.z; vNorm.push_back(n); }
        else if (kw == "usemtl") {
            string mname; ss >> mname;
            if (mname != groups.back().matName) {
                if (!groups.back().buf.empty())
                    groups.push_back({mname, {}});
                else
                    groups.back().matName = mname;
            }
        }
        else if (kw == "f") {
            vector<array<int,3>> face;
            string tok;
            while (ss >> tok) {
                array<int,3> idx{-1,-1,-1};
                istringstream sw(tok); string part;
                if (getline(sw,part,'/')) idx[0] = part.empty() ? -1 : stoi(part)-1;
                if (getline(sw,part,'/')) idx[1] = part.empty() ? -1 : stoi(part)-1;
                if (getline(sw,part))    idx[2] = part.empty() ? -1 : stoi(part)-1;
                face.push_back(idx);
            }

            auto pushV = [&](const array<int,3> &idx) {
                auto &buf = groups.back().buf;
                glm::vec3 p = (idx[0]>=0 && idx[0]<(int)vPos.size())  ? vPos[idx[0]]  : glm::vec3(0);
                buf.push_back(p.x); buf.push_back(p.y); buf.push_back(p.z);
                if (!vUV.empty() && idx[1]>=0 && idx[1]<(int)vUV.size()) {
                    buf.push_back(vUV[idx[1]].s);
                    buf.push_back(1.0f - vUV[idx[1]].t);
                } else { buf.push_back(0.0f); buf.push_back(0.0f); }
                if (!vNorm.empty() && idx[2]>=0 && idx[2]<(int)vNorm.size()) {
                    buf.push_back(vNorm[idx[2]].x); buf.push_back(vNorm[idx[2]].y); buf.push_back(vNorm[idx[2]].z);
                } else { buf.push_back(0.0f); buf.push_back(1.0f); buf.push_back(0.0f); }
            };

            // Fan triangulation
            for (int i = 1; i+1 < (int)face.size(); ++i) {
                pushV(face[0]); pushV(face[i]); pushV(face[i+1]);
            }
        }
    }
    f.close();

    // Carrega materiais do MTL
    if (!mtlFile.empty()) {
        string mp = resolve(objDir, mtlFile);
        ifstream mf(mp);
        if (!mf.is_open()) {
            cerr << "  Aviso: MTL nao encontrado: " << mp << "\n";
        } else {
            string curName;
            Material curMat;
            string ml;
            while (getline(mf, ml)) {
                if (!ml.empty() && ml.back() == '\r') ml.pop_back();
                if (ml.empty() || ml[0] == '#') continue;
                istringstream ms(ml); string mk; ms >> mk;
                if (mk == "newmtl") {
                    if (!curName.empty()) mats[curName] = curMat;
                    ms >> curName; curMat = {};
                }
                // Ka/Kd/Ks podem ser vec3; usamos a media como coeficiente escalar
                else if (mk == "Ka") { float r,g,b; ms>>r>>g>>b; curMat.ka = (r+g+b)/3.0f; }
                else if (mk == "Kd") { float r,g,b; ms>>r>>g>>b; curMat.kd = (r+g+b)/3.0f; }
                else if (mk == "Ks") { float r,g,b; ms>>r>>g>>b; curMat.ks = (r+g+b)/3.0f; }
                else if (mk == "Ns") { ms >> curMat.shininess; }
                else if (mk == "map_Kd") {
                    string tn; ms >> tn;
                    curMat.texPath = resolve(objDir, tn);
                }
            }
            if (!curName.empty()) mats[curName] = curMat;
        }
    }

    // Cria um SubMesh por grupo com faces
    for (auto &g : groups) {
        if (g.buf.empty()) continue;

        SubMesh sm;
        if (mats.count(g.matName)) sm.mat = mats[g.matName];

        string texPath = sm.mat.texPath.empty() ? fallbackTex : sm.mat.texPath;
        if (!texPath.empty()) sm.tex.load(texPath);

        const GLsizei stride = 8 * sizeof(GLfloat);
        glGenVertexArrays(1, &sm.VAO);
        glGenBuffers(1, &sm.VBO);
        glBindVertexArray(sm.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, sm.VBO);
        glBufferData(GL_ARRAY_BUFFER, g.buf.size()*sizeof(GLfloat), g.buf.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3*sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5*sizeof(GLfloat)));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);

        sm.nVerts = (int)(g.buf.size() / 8);
        cout << "    submesh [" << g.matName << "]: " << sm.nVerts << " vertices\n";
        out.push_back(move(sm));
    }
    return !out.empty();
}

// ---- Parser do arquivo de configuracao de cena ------------------------------
//
// Formato (secoes com [nome], pares chave valor):
//
//   [camera]
//   position 0 1 5
//   yaw -90
//   pitch -10
//   fov 45
//   near 0.1
//   far 100
//
//   [light]
//   position 2 3 5
//   color 1 1 1
//   intensity 1.0
//
//   [object]
//   name Suzanne
//   file ../assets/Modelos3D/Suzanne.obj
//   position -1 0 0
//   scale 0.6 0.6 0.6
//   rotation 0 180 0
//   trajectory ../assets/trajetoria_suzanne.txt
//   trajspeed 0.8

struct SceneDef {
    // Camera
    glm::vec3 camPos{0,0,5};
    float camYaw=-90, camPitch=0, camFov=45, camNear=0.1f, camFar=100;

    // Luzes
    vector<Light> lights;

    // Objetos
    struct ObjSpec {
        string name = "Objeto";
        string file;
        string trajectoryFile;
        glm::vec3 position{0};
        glm::vec3 scale{1};
        glm::vec3 rotation{0};
        float     trajSpeed = 0.8f;
    };
    vector<ObjSpec> objects;
};

SceneDef parseScene(const string &path) {
    SceneDef def;
    ifstream f(path);
    if (!f.is_open()) {
        cerr << "Arquivo de cena nao encontrado: " << path << "\n";
        return def;
    }

    string section;
    Light           curLight;
    SceneDef::ObjSpec curObj;
    bool inLight = false, inObj = false;

    auto flushLight = [&]() {
        if (inLight) { def.lights.push_back(curLight); curLight = {}; inLight = false; }
    };
    auto flushObj = [&]() {
        if (inObj && !curObj.file.empty()) { def.objects.push_back(curObj); curObj = {}; inObj = false; }
    };

    string line;
    while (getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        // Remove comentario
        size_t c = line.find('#');
        if (c != string::npos) line = line.substr(0, c);
        // Trim
        while (!line.empty() && isspace((unsigned char)line.front())) line.erase(line.begin());
        while (!line.empty() && isspace((unsigned char)line.back()))  line.pop_back();
        if (line.empty()) continue;

        if (line.front() == '[') {
            flushLight(); flushObj();
            size_t end = line.find(']');
            section = toLower(line.substr(1, end == string::npos ? line.size()-1 : end-1));
            if (section == "light")  { inLight = true; curLight = {}; }
            if (section == "object") { inObj   = true; curObj   = {}; }
            continue;
        }

        istringstream ss(line);
        string key; ss >> key;
        key = toLower(key);

        if (section == "camera") {
            if      (key == "position") ss >> def.camPos.x  >> def.camPos.y  >> def.camPos.z;
            else if (key == "yaw")      ss >> def.camYaw;
            else if (key == "pitch")    ss >> def.camPitch;
            else if (key == "fov")      ss >> def.camFov;
            else if (key == "near")     ss >> def.camNear;
            else if (key == "far")      ss >> def.camFar;
        }
        else if (section == "light" && inLight) {
            if      (key == "position")  ss >> curLight.position.x >> curLight.position.y >> curLight.position.z;
            else if (key == "color")     ss >> curLight.color.r    >> curLight.color.g    >> curLight.color.b;
            else if (key == "intensity") ss >> curLight.intensity;
        }
        else if (section == "object" && inObj) {
            if      (key == "name")       { string v; ss>>v; curObj.name=v; }
            else if (key == "file")       { string v; ss>>v; curObj.file=v; }
            else if (key == "trajectory") { string v; ss>>v; curObj.trajectoryFile=v; }
            else if (key == "trajspeed")  ss >> curObj.trajSpeed;
            else if (key == "position")   ss >> curObj.position.x >> curObj.position.y >> curObj.position.z;
            else if (key == "rotation")   ss >> curObj.rotation.x >> curObj.rotation.y >> curObj.rotation.z;
            else if (key == "scale") {
                float sx; ss >> sx;
                float sy = sx, sz = sx;
                ss >> sy >> sz;  // tenta ler y e z; se falhar mantem sx
                curObj.scale = glm::vec3(sx, sy, sz);
            }
        }
    }
    flushLight(); flushObj();
    return def;
}

// ---- Estado global ----------------------------------------------------------

Camera           gCamera;
vector<SceneObject> gObjects;
vector<Light>    gLights;
int              gSelected   = 0;
bool             gRotX=false, gRotY=false, gRotZ=false;
float            gDelta=0.0f, gLastFrame=0.0f;
GLuint           gShader = 0;

const float STEP_T = 0.1f;   // translacao
const float STEP_S = 0.05f;  // escala
const float MIN_S  = 0.05f;

// ---- Prototipos -------------------------------------------------------------

void keyCallback   (GLFWwindow*, int, int, int, int);
void mouseCallback (GLFWwindow*, double, double);
void scrollCallback(GLFWwindow*, double, double);
GLuint compileShaders();
void   uploadLights(GLuint shader);

// ---- main -------------------------------------------------------------------

int main(int argc, char **argv) {
    // Tenta varios caminhos relativos para o scene.txt
    // (o exe pode estar em build/ ou build/Release/ dependendo do gerador)
    string sceneFile = "../assets/scene.txt";
    if (argc > 1) {
        sceneFile = argv[1];
    } else {
        ifstream test("../assets/scene.txt");
        if (!test.is_open()) sceneFile = "../../assets/scene.txt";
    }

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *win = glfwCreateWindow(WIN_W, WIN_H, "Visualizador de Cenas 3D", nullptr, nullptr);
    if (!win) { cerr << "GLFW: falha ao criar janela\n"; return -1; }
    glfwMakeContextCurrent(win);
    glfwSetKeyCallback(win,     keyCallback);
    glfwSetCursorPosCallback(win, mouseCallback);
    glfwSetScrollCallback(win,  scrollCallback);
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cerr << "GLAD: falha na inicializacao\n"; return -1;
    }
    cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";
    cout << "OpenGL:   " << glGetString(GL_VERSION)  << "\n\n";

    int fbW, fbH;
    glfwGetFramebufferSize(win, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);
    glEnable(GL_DEPTH_TEST);

    gShader = compileShaders();
    glUseProgram(gShader);
    glUniform1i(glGetUniformLocation(gShader, "texBuff"), 0);

    // --- Carrega e interpreta o arquivo de cena ---
    cout << "Carregando cena: " << sceneFile << "\n";
    SceneDef scene = parseScene(sceneFile);
    string sceneDir = dirOf(sceneFile);

    // Configura camera com valores do arquivo
    gCamera.position  = scene.camPos;
    gCamera.yaw       = scene.camYaw;
    gCamera.pitch     = scene.camPitch;
    gCamera.fov       = scene.camFov;
    gCamera.nearP     = scene.camNear;
    gCamera.farP      = scene.camFar;
    gCamera.init();

    // Luzes (pelo menos 1 padrao se nenhuma for especificada)
    gLights = scene.lights;
    if (gLights.empty()) {
        Light l; gLights.push_back(l);
        cout << "Aviso: nenhuma luz no arquivo de cena; usando padrao.\n";
    }

    // Carrega cada objeto especificado na cena
    string fallback = resolve(sceneDir, "tex/pixelWall.png");
    for (int i = 0; i < (int)scene.objects.size(); ++i) {
        auto &spec = scene.objects[i];
        SceneObject obj;
        obj.name            = spec.name;
        obj.position        = spec.position;
        obj.scale           = spec.scale;
        obj.baseRot         = spec.rotation;
        obj.trajectory.speed = spec.trajSpeed;

        // Arquivo de trajetoria: explicito no arquivo de cena ou nome automatico
        obj.trajectoryFile = spec.trajectoryFile.empty()
            ? resolve(sceneDir, "../assets/trajetoria_" + to_string(i) + ".txt")
            : resolve(sceneDir, spec.trajectoryFile);

        string objPath = resolve(sceneDir, spec.file);
        cout << "OBJ: " << objPath << "\n";
        if (!loadOBJ(objPath, obj.submeshes, fallback)) {
            cerr << "  ERRO: falha ao carregar " << objPath << "\n";
            continue;
        }

        if (obj.trajectory.loadFromFile(obj.trajectoryFile)) {
            cout << "  Trajetoria carregada: " << obj.trajectory.points.size() << " pontos\n";
            obj.trajectory.active = true;  // ativa automaticamente se vier do arquivo de cena
        }

        gObjects.push_back(move(obj));
    }

    if (gObjects.empty()) {
        cerr << "Nenhum objeto carregado. Verifique " << sceneFile << "\n";
        return -1;
    }

    // Envia luzes iniciais ao shader
    uploadLights(gShader);

    // Locations de uniforms fixos
    GLint modelLoc  = glGetUniformLocation(gShader, "model");
    GLint viewLoc   = glGetUniformLocation(gShader, "view");
    GLint projLoc   = glGetUniformLocation(gShader, "projection");
    GLint normLoc   = glGetUniformLocation(gShader, "normalMatrix");
    GLint camLoc    = glGetUniformLocation(gShader, "cameraPos");
    GLint kaLoc     = glGetUniformLocation(gShader, "ka");
    GLint kdLoc     = glGetUniformLocation(gShader, "kd");
    GLint ksLoc     = glGetUniformLocation(gShader, "ks");
    GLint shineLoc  = glGetUniformLocation(gShader, "shininess");

    cout << "\n=== Controles ===\n";
    cout << "1-9      Seleciona objeto (" << gObjects.size() << " carregado(s))\n";
    cout << "Setas    Move objeto XY  |  PgUp/Dn: move Z  (trajetoria inativa)\n";
    cout << "+/-      Escala (numpad)\n";
    cout << "X/Y/Z    Rotacao automatica  |  P: para rotacao\n";
    cout << "F        Registra ponto de trajetoria\n";
    cout << "T        Ativa/desativa trajetoria\n";
    cout << "C        Limpa trajetoria\n";
    cout << "M        Salva trajetoria em arquivo\n";
    cout << "I/O      Aumenta/diminui intensidade das luzes\n";
    cout << "WASD     Camera  |  Mouse: orienta  |  Scroll: zoom\n";
    cout << "ESC      Sai\n\n";

    // ---- Loop principal -----------------------------------------------------
    while (!glfwWindowShouldClose(win)) {
        float now = (float)glfwGetTime();
        gDelta     = now - gLastFrame;
        gLastFrame = now;

        gCamera.processKeyboard(win, gDelta);
        glfwPollEvents();

        // Atualiza trajetorias e reposiciona objetos
        for (auto &obj : gObjects) {
            if (obj.trajectory.active && obj.trajectory.hasPath()) {
                obj.trajectory.update(gDelta);
                obj.position = obj.trajectory.currentPos();
            }
        }

        // Rotacao automatica do objeto selecionado
        if (gSelected < (int)gObjects.size()) {
            float angle = now * 50.0f;
            auto &rot = gObjects[gSelected].rotation;
            if      (gRotX) rot = {angle, 0, 0};
            else if (gRotY) rot = {0, angle, 0};
            else if (gRotZ) rot = {0, 0, angle};
        }

        glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Uniforms de camera
        int fw, fh; glfwGetFramebufferSize(win, &fw, &fh);
        float aspect = (fh > 0) ? (float)fw / fh : 1.0f;
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(gCamera.viewMatrix()));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(gCamera.projMatrix(aspect)));
        glUniform3fv(camLoc, 1, glm::value_ptr(gCamera.position));

        // Desenha cada objeto com seus submeshes
        for (auto &obj : gObjects) {
            glm::vec3 rot = obj.baseRot + obj.rotation;
            glm::mat4 model(1.0f);
            model = glm::translate(model, obj.position);
            model = glm::rotate(model, glm::radians(rot.x), {1,0,0});
            model = glm::rotate(model, glm::radians(rot.y), {0,1,0});
            model = glm::rotate(model, glm::radians(rot.z), {0,0,1});
            model = glm::scale(model, obj.scale);

            // Matriz normal correta: transposta da inversa da parte 3x3 do model
            glm::mat3 normMat = glm::transpose(glm::inverse(glm::mat3(model)));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix3fv(normLoc,  1, GL_FALSE, glm::value_ptr(normMat));

            for (auto &sm : obj.submeshes) {
                glUniform1f(kaLoc,    sm.mat.ka);
                glUniform1f(kdLoc,    sm.mat.kd);
                glUniform1f(ksLoc,    sm.mat.ks);
                glUniform1f(shineLoc, sm.mat.shininess);
                sm.draw();
            }
        }

        glfwSwapBuffers(win);
    }

    for (auto &obj : gObjects) obj.release();
    glfwTerminate();
    return 0;
}

// ---- Callbacks --------------------------------------------------------------

void keyCallback(GLFWwindow *win, int key, int /*sc*/, int action, int /*mods*/) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(win, GL_TRUE);

    // Selecao de objeto (teclas 1-9) - exibe material ao selecionar
    if (action == GLFW_PRESS && key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
        int idx = key - GLFW_KEY_1;
        if (idx < (int)gObjects.size()) {
            gSelected = idx;
            gRotX = gRotY = gRotZ = false;
            auto &obj = gObjects[gSelected];
            cout << "\nSelecionado: " << obj.name << "\n";
            if (!obj.submeshes.empty()) {
                auto &m = obj.submeshes[0].mat;
                cout << "  Material: ka=" << m.ka
                     << "  kd=" << m.kd
                     << "  ks=" << m.ks
                     << "  shininess=" << m.shininess << "\n";
            }
        }
    }

    // Toggle individual de luzes: [ ] \ (pontos key/fill/back)
    if (action == GLFW_PRESS) {
        int lightIdx = -1;
        if (key == GLFW_KEY_LEFT_BRACKET)  lightIdx = 0;
        if (key == GLFW_KEY_RIGHT_BRACKET) lightIdx = 1;
        if (key == GLFW_KEY_BACKSLASH)     lightIdx = 2;
        if (lightIdx >= 0 && lightIdx < (int)gLights.size()) {
            gLights[lightIdx].enabled = !gLights[lightIdx].enabled;
            const char *names[] = {"Key light", "Fill light", "Back light"};
            cout << names[lightIdx] << ": "
                 << (gLights[lightIdx].enabled ? "ON" : "OFF") << "\n";
            uploadLights(gShader);
        }
    }

    // Eixos de rotacao
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_X) { gRotX=true;  gRotY=gRotZ=false; }
        if (key == GLFW_KEY_Y) { gRotY=true;  gRotX=gRotZ=false; }
        if (key == GLFW_KEY_Z) { gRotZ=true;  gRotX=gRotY=false; }
        if (key == GLFW_KEY_P) { gRotX=gRotY=gRotZ=false; }
    }

    // Intensidade das luzes
    bool held = (action == GLFW_PRESS || action == GLFW_REPEAT);
    if (key == GLFW_KEY_I && held) {
        for (auto &l : gLights) l.intensity = glm::min(l.intensity + 0.1f, 5.0f);
        uploadLights(gShader);
        if (!gLights.empty()) cout << "Intensidade luz: " << gLights[0].intensity << "\n";
    }
    if (key == GLFW_KEY_O && held) {
        for (auto &l : gLights) l.intensity = glm::max(l.intensity - 0.1f, 0.0f);
        uploadLights(gShader);
        if (!gLights.empty()) cout << "Intensidade luz: " << gLights[0].intensity << "\n";
    }

    if (gSelected >= (int)gObjects.size()) return;
    SceneObject &obj = gObjects[gSelected];

    // Trajetoria
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        obj.trajectory.addPoint(obj.position);
        cout << obj.name << ": " << obj.trajectory.points.size() << " ponto(s) de trajetoria\n";
    }
    if (key == GLFW_KEY_T && action == GLFW_PRESS) {
        if (!obj.trajectory.hasPath())
            cout << obj.name << ": adicione >= 2 pontos com F primeiro\n";
        else {
            obj.trajectory.active = !obj.trajectory.active;
            cout << obj.name << ": trajetoria " << (obj.trajectory.active ? "ATIVADA" : "DESATIVADA") << "\n";
        }
    }
    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        obj.trajectory.clear();
        cout << obj.name << ": trajetoria limpa\n";
    }
    if (key == GLFW_KEY_M && action == GLFW_PRESS) {
        if (obj.trajectory.points.empty())
            cout << obj.name << ": trajetoria vazia, nada a salvar\n";
        else {
            obj.trajectory.saveToFile(obj.trajectoryFile);
            cout << obj.name << ": trajetoria salva em " << obj.trajectoryFile << "\n";
        }
    }

    // Transformacoes manuais (apenas quando trajetoria inativa)
    if (!obj.trajectory.active) {
        if (key == GLFW_KEY_UP        && held) obj.position.y += STEP_T;
        if (key == GLFW_KEY_DOWN      && held) obj.position.y -= STEP_T;
        if (key == GLFW_KEY_LEFT      && held) obj.position.x -= STEP_T;
        if (key == GLFW_KEY_RIGHT     && held) obj.position.x += STEP_T;
        if (key == GLFW_KEY_PAGE_UP   && held) obj.position.z += STEP_T;
        if (key == GLFW_KEY_PAGE_DOWN && held) obj.position.z -= STEP_T;
        if (key == GLFW_KEY_KP_ADD      && held) obj.scale += glm::vec3(STEP_S);
        if (key == GLFW_KEY_KP_SUBTRACT && held) {
            obj.scale -= glm::vec3(STEP_S);
            obj.scale  = glm::max(obj.scale, glm::vec3(MIN_S));
        }
    }
}

void mouseCallback(GLFWwindow * /*w*/, double x, double y) { gCamera.processMouse(x, y); }
void scrollCallback(GLFWwindow * /*w*/, double /*x*/, double y) { gCamera.processScroll(y); }

// ---- Carrega uniformes de todas as luzes ------------------------------------

void uploadLights(GLuint shader) {
    glUseProgram(shader);
    int slot = 0;
    for (int i = 0; i < (int)gLights.size() && slot < MAX_LIGHTS; ++i) {
        if (!gLights[i].enabled) continue;
        string pi = "lightPos["       + to_string(slot) + "]";
        string ci = "lightColor["     + to_string(slot) + "]";
        string ii = "lightIntensity[" + to_string(slot) + "]";
        glUniform3fv(glGetUniformLocation(shader, pi.c_str()), 1, glm::value_ptr(gLights[i].position));
        glUniform3fv(glGetUniformLocation(shader, ci.c_str()), 1, glm::value_ptr(gLights[i].color));
        glUniform1f (glGetUniformLocation(shader, ii.c_str()),    gLights[i].intensity);
        ++slot;
    }
    glUniform1i(glGetUniformLocation(shader, "numLights"), slot);
}

// ---- Compilacao dos shaders -------------------------------------------------

GLuint compileShaders() {
    auto compile = [](GLenum type, const char *src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024]; glGetShaderInfoLog(s, 1024, nullptr, log);
            cerr << "Shader error:\n" << log << "\n";
        }
        return s;
    };
    GLuint vs = compile(GL_VERTEX_SHADER,   VERT_SRC);
    GLuint fs = compile(GL_FRAGMENT_SHADER, FRAG_SRC);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { char log[1024]; glGetProgramInfoLog(prog, 1024, nullptr, log); cerr << log << "\n"; }
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}
