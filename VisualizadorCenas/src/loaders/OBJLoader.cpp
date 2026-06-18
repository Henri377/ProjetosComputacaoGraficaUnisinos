#include "OBJLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <array>
#include <algorithm>
#include <functional>
#include <unordered_map>

// ---- utilitarios de caminho ------------------------------------------------

/// @brief Retorna o diretorio de um caminho de arquivo, incluindo o separador final.
/// @param p Caminho completo do arquivo.
/// @return Prefixo de diretorio, ou "" se nao houver separador.
std::string dirOf(const std::string& p)
{
    size_t pos = p.find_last_of("/\\");
    return (pos == std::string::npos) ? "" : p.substr(0, pos + 1);
}

/**
 * @brief Resolve um caminho relativo combinando-o com um diretorio base.
 * @param base Diretorio base (geralmente obtido via dirOf).
 * @param rel  Caminho relativo ou absoluto a ser resolvido.
 * @return Caminho resultante, ou "" se rel estiver vazio.
 */
std::string resolvePath(const std::string& base, const std::string& rel)
{
    if (rel.empty()) return "";
    bool absolute = rel.find(':') != std::string::npos || rel[0] == '/' || rel[0] == '\\';
    return absolute ? rel : base + rel;
}

// ---- helpers internos -------------------------------------------------------

/**
 * @brief Converte um token de face OBJ "vi/ti/ni" (1-based) para indices 0-based.
 * @param token Token de vertice de face no formato "v", "v/t", "v//n" ou "v/t/n".
 * @return Array de tres inteiros {posicao, textura, normal}; -1 onde ausente.
 */
static std::array<int,3> parseFaceVertex(const std::string& token)
{
    std::array<int,3> idx{-1,-1,-1};
    std::istringstream ss(token);
    std::string part;
    if (std::getline(ss, part, '/') && !part.empty()) idx[0] = std::stoi(part) - 1;
    if (std::getline(ss, part, '/') && !part.empty()) idx[1] = std::stoi(part) - 1;
    if (std::getline(ss, part)      && !part.empty()) idx[2] = std::stoi(part) - 1;
    return idx;
}

/**
 * @brief Agrupamento de referencias aos buffers de atributos geometricos do OBJ.
 *
 * Usado como parametro de passagem para evitar copias dos vetores de pos/uv/norm.
 */
struct VertexData {
    /// @brief Posicoes de vertices lidas do OBJ (linhas "v").
    const std::vector<glm::vec3>& pos;
    /// @brief Coordenadas de textura lidas do OBJ (linhas "vt").
    const std::vector<glm::vec2>& uv;
    /// @brief Normais de vertices lidas do OBJ (linhas "vn").
    const std::vector<glm::vec3>& norm;
};

/**
 * @brief Empurra os atributos de um vertice (pos + uv + normal) no buffer interleaved.
 * @param idx Indices 0-based de {posicao, textura, normal}; -1 indica ausencia.
 * @param buf Buffer de saida onde os 8 floats do vertice serao anexados.
 * @param vd  Referencias aos arrays de atributos do OBJ.
 */
static void pushVertex(const std::array<int,3>& idx, std::vector<GLfloat>& buf,
                       const VertexData& vd)
{
    glm::vec3 p = (idx[0] >= 0 && idx[0] < (int)vd.pos.size()) ? vd.pos[idx[0]] : glm::vec3(0);
    buf.insert(buf.end(), {p.x, p.y, p.z});

    bool hasUV = !vd.uv.empty() && idx[1] >= 0 && idx[1] < (int)vd.uv.size();
    buf.push_back(hasUV ? vd.uv[idx[1]].s       : 0.0f);
    buf.push_back(hasUV ? 1.0f - vd.uv[idx[1]].t : 0.0f);

    bool hasNorm = !vd.norm.empty() && idx[2] >= 0 && idx[2] < (int)vd.norm.size();
    buf.push_back(hasNorm ? vd.norm[idx[2]].x : 0.0f);
    buf.push_back(hasNorm ? vd.norm[idx[2]].y : 1.0f);
    buf.push_back(hasNorm ? vd.norm[idx[2]].z : 0.0f);
}

/**
 * @brief Faz parse de uma linha "f" do OBJ e triangula a face por fan, anexando ao buffer.
 * @param ss  Stream posicionado apos o token "f".
 * @param buf Buffer interleaved de saida onde os vertices triangulados serao anexados.
 * @param vd  Referencias aos arrays de atributos do OBJ.
 */
static void parseFace(std::istringstream& ss, std::vector<GLfloat>& buf, const VertexData& vd)
{
    std::vector<std::array<int,3>> face;
    std::string tok;
    while (ss >> tok) face.push_back(parseFaceVertex(tok));

    for (int i = 1; i + 1 < (int)face.size(); ++i) {   // fan triangulation
        pushVertex(face[0], buf, vd);
        pushVertex(face[i], buf, vd);
        pushVertex(face[i+1], buf, vd);
    }
}

/**
 * @brief Cria uma SubMesh a partir de um buffer interleaved e um material.
 * @param buf     Buffer de vertices interleaved (pos3 + uv2 + norm3 por vertice).
 * @param mat     Material Phong a ser associado a SubMesh.
 * @param texPath Caminho da textura a ser carregada; vazio = sem textura.
 * @return SubMesh com VAO/VBO configurados e pronta para renderizacao.
 */
static SubMesh buildSubMesh(const std::vector<GLfloat>& buf, const Material& mat,
                            const std::string& texPath)
{
    SubMesh sm;
    sm.mat = mat;
    if (!texPath.empty()) sm.tex.load(texPath);

    const GLsizei stride = 8 * sizeof(GLfloat);
    glGenVertexArrays(1, &sm.VAO);
    glGenBuffers(1, &sm.VBO);
    glBindVertexArray(sm.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, sm.VBO);
    glBufferData(GL_ARRAY_BUFFER, buf.size() * sizeof(GLfloat), buf.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    sm.nVerts = (int)(buf.size() / 8);
    return sm;
}

// ---- MTL parser -------------------------------------------------------------

/**
 * @brief Faz parse de um arquivo MTL e retorna um mapa de materiais por nome.
 * @param path   Caminho absoluto para o arquivo .mtl.
 * @param objDir Diretorio do OBJ, usado para resolver caminhos de texturas relativos.
 * @return Mapa de nome do material para struct Material; vazio se o arquivo nao for encontrado.
 */
static std::map<std::string, Material> parseMTL(const std::string& path,
                                                const std::string& objDir)
{
    std::map<std::string, Material> mats;
    std::ifstream f(path);
    if (!f.is_open()) { std::cerr << "  Aviso: MTL nao encontrado: " << path << "\n"; return mats; }

    std::string curName;
    Material    curMat;

    auto flush = [&]() { if (!curName.empty()) mats[curName] = curMat; };

    auto readRGB = [](std::istringstream& ss) {
        float r, g, b; ss >> r >> g >> b;
        return (r + g + b) / 3.0f;
    };

    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string kw; ss >> kw;

        using Fn = std::function<void(std::istringstream&)>;
        const std::unordered_map<std::string, Fn> dispatch = {
            {"newmtl",  [&](auto& s) { flush(); s >> curName; curMat = {}; }},
            {"Ka",      [&](auto& s) { curMat.ka        = readRGB(s); }},
            {"Kd",      [&](auto& s) { curMat.kd        = readRGB(s); }},
            {"Ks",      [&](auto& s) { curMat.ks        = readRGB(s); }},
            {"Ns",      [&](auto& s) { s >> curMat.shininess; }},
            {"map_Kd",  [&](auto& s) { std::string tn; s >> tn;
                                        curMat.texPath = resolvePath(objDir, tn); }},
        };

        auto it = dispatch.find(kw);
        if (it != dispatch.end()) it->second(ss);
    }
    flush();
    return mats;
}

// ---- OBJ loader principal --------------------------------------------------

/**
 * @brief Carrega um arquivo OBJ e popula a lista de SubMeshes.
 * @param objPath     Caminho absoluto ou relativo ao arquivo .obj.
 * @param out         Vetor onde as SubMeshes geradas serao adicionadas.
 * @param fallbackTex Textura usada quando o MTL nao especifica map_Kd.
 * @return true se ao menos uma SubMesh foi carregada com sucesso.
 */
bool loadOBJ(const std::string& objPath, std::vector<SubMesh>& out,
             const std::string& fallbackTex)
{
    std::vector<glm::vec3> vPos;
    std::vector<glm::vec2> vUV;
    std::vector<glm::vec3> vNorm;
    std::string             objDir = dirOf(objPath);
    std::string             mtlFile;

    struct Group { std::string matName; std::vector<GLfloat> buf; };
    std::vector<Group> groups{{"default", {}}};

    std::ifstream f(objPath);
    if (!f.is_open()) { std::cerr << "OBJ nao encontrado: " << objPath << "\n"; return false; }

    auto switchGroup = [&](const std::string& name) {
        if (name == groups.back().matName) return;
        if (groups.back().buf.empty()) { groups.back().matName = name; return; }
        groups.push_back({name, {}});
    };

    using Fn = std::function<void(std::istringstream&)>;
    const std::unordered_map<std::string, Fn> dispatch = {
        {"v",      [&](auto& ss) { glm::vec3 v; ss>>v.x>>v.y>>v.z; vPos.push_back(v); }},
        {"vt",     [&](auto& ss) { glm::vec2 u; ss>>u.s>>u.t;      vUV.push_back(u); }},
        {"vn",     [&](auto& ss) { glm::vec3 n; ss>>n.x>>n.y>>n.z; vNorm.push_back(n); }},
        {"mtllib", [&](auto& ss) { ss >> mtlFile; }},
        {"usemtl", [&](auto& ss) { std::string n; ss>>n; switchGroup(n); }},
        {"f",      [&](auto& ss) { parseFace(ss, groups.back().buf, {vPos, vUV, vNorm}); }},
    };

    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string kw; ss >> kw;
        auto it = dispatch.find(kw);
        if (it != dispatch.end()) it->second(ss);
    }

    auto mats = mtlFile.empty()
        ? std::map<std::string, Material>{}
        : parseMTL(resolvePath(objDir, mtlFile), objDir);

    for (const auto& g : groups) {
        if (g.buf.empty()) continue;
        Material mat = mats.count(g.matName) ? mats.at(g.matName) : Material{};
        std::string tex = mat.texPath.empty() ? fallbackTex : mat.texPath;
        SubMesh sm = buildSubMesh(g.buf, mat, tex);
        std::cout << "    submesh [" << g.matName << "]: " << sm.nVerts << " vertices\n";
        out.push_back(std::move(sm));
    }
    return !out.empty();
}
