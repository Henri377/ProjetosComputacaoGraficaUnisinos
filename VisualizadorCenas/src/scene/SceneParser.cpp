#include "SceneParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

// ---- utilitarios -----------------------------------------------------------

/// @brief Converte uma string para letras minusculas.
/// @param s String a ser convertida.
/// @return Copia da string em minusculas.
static std::string toLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

/**
 * @brief Remove comentarios, carriage returns e espacos em branco de uma linha.
 * @param line Linha bruta lida do arquivo.
 * @return Linha sem comentario '#', sem '\r' e sem espacos nas extremidades.
 */
static std::string trimLine(std::string line)
{
    size_t c = line.find('#');
    if (c != std::string::npos) line.resize(c);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    line.erase(line.begin(), std::find_if(line.begin(), line.end(), notSpace));
    line.erase(std::find_if(line.rbegin(), line.rend(), notSpace).base(), line.end());
    return line;
}

// ---- parsers por secao -----------------------------------------------------

/**
 * @brief Aplica um par chave-valor da secao [camera] ao SceneDef.
 * @param def SceneDef que sera preenchida.
 * @param key Chave em minusculas (ex.: "position", "yaw", "fov").
 * @param ss  Stream com o restante da linha apos a chave.
 */
static void applyCamera(SceneDef& def, const std::string& key, std::istringstream& ss)
{
    if (key == "position") { ss >> def.camPos.x >> def.camPos.y >> def.camPos.z; return; }
    if (key == "yaw")      { ss >> def.camYaw;   return; }
    if (key == "pitch")    { ss >> def.camPitch;  return; }
    if (key == "fov")      { ss >> def.camFov;    return; }
    if (key == "near")     { ss >> def.camNear;   return; }
    if (key == "far")      { ss >> def.camFar;    return; }
}

/**
 * @brief Aplica um par chave-valor da secao [light] a uma struct Light.
 * @param light Luz que sera configurada.
 * @param key   Chave em minusculas (ex.: "position", "color", "intensity").
 * @param ss    Stream com o restante da linha apos a chave.
 */
static void applyLight(Light& light, const std::string& key, std::istringstream& ss)
{
    if (key == "position")  { ss >> light.position.x >> light.position.y >> light.position.z; return; }
    if (key == "color")     { ss >> light.color.r    >> light.color.g    >> light.color.b;    return; }
    if (key == "intensity") { ss >> light.intensity; return; }
}

/**
 * @brief Aplica um par chave-valor da secao [object] a uma ObjSpec.
 * @param obj ObjSpec que sera configurada.
 * @param key Chave em minusculas (ex.: "name", "file", "position", "scale", "rotation").
 * @param ss  Stream com o restante da linha apos a chave.
 */
static void applyObject(SceneDef::ObjSpec& obj, const std::string& key, std::istringstream& ss)
{
    if (key == "name")       { ss >> obj.name;            return; }
    if (key == "file")       { ss >> obj.file;            return; }
    if (key == "trajectory") { ss >> obj.trajectoryFile;  return; }
    if (key == "trajspeed")  { ss >> obj.trajSpeed;       return; }
    if (key == "position")   { ss >> obj.position.x >> obj.position.y >> obj.position.z; return; }
    if (key == "rotation")   { ss >> obj.rotation.x >> obj.rotation.y >> obj.rotation.z; return; }
    if (key == "scale") {
        float sx; ss >> sx;
        float sy = sx, sz = sx;
        ss >> sy >> sz;
        obj.scale = glm::vec3(sx, sy, sz);
    }
}

// ---- parser principal ------------------------------------------------------

/**
 * @brief Faz parse de um arquivo de cena no formato INI por secoes.
 * @param path Caminho para o arquivo de cena.
 * @return SceneDef preenchida; valores padrao se o arquivo nao for encontrado.
 */
SceneDef parseScene(const std::string& path)
{
    SceneDef def;
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "Arquivo de cena nao encontrado: " << path << "\n";
        return def;
    }

    std::string       section;
    Light             curLight;
    SceneDef::ObjSpec curObj;
    bool inLight = false, inObj = false;

    auto flushLight = [&]() {
        if (!inLight) return;
        def.lights.push_back(curLight);
        curLight = {}; inLight = false;
    };
    auto flushObj = [&]() {
        if (!inObj || curObj.file.empty()) return;
        def.objects.push_back(curObj);
        curObj = {}; inObj = false;
    };

    std::string raw;
    while (std::getline(f, raw)) {
        std::string line = trimLine(raw);
        if (line.empty()) continue;

        if (line.front() == '[') {
            flushLight(); flushObj();
            size_t end = line.find(']');
            section = toLower(line.substr(1, end == std::string::npos ? line.size()-1 : end-1));
            inLight = (section == "light");
            inObj   = (section == "object");
            if (inLight) curLight = {};
            if (inObj)   curObj   = {};
            continue;
        }

        std::istringstream ss(line);
        std::string key; ss >> key;
        key = toLower(key);

        if (section == "camera")           applyCamera(def,      key, ss);
        else if (section == "light" && inLight)  applyLight(curLight,  key, ss);
        else if (section == "object" && inObj)   applyObject(curObj,   key, ss);
    }
    flushLight(); flushObj();
    return def;
}
