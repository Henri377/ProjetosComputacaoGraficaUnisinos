#include "Trajectory.h"
#include <fstream>
#include <sstream>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

/// @brief Avalia a posicao na spline Catmull-Rom (via Bezier cubico) para o segmento atual.
/// @return Ponto 3D interpolado na posicao t do segmento idx.
glm::vec3 Trajectory::currentPos() const
{
    if (points.empty())          return {};
    if ((int)points.size() == 1) return points[0];

    int N  = (int)points.size();
    glm::vec3 p0 = points[((idx - 1) + N) % N];
    glm::vec3 p1 = points[idx];
    glm::vec3 p2 = points[(idx + 1) % N];
    glm::vec3 p3 = points[(idx + 2) % N];

    glm::vec3 b0 = p1;
    glm::vec3 b1 = p1 + (p2 - p0) / 6.0f;
    glm::vec3 b2 = p2 - (p3 - p1) / 6.0f;
    glm::vec3 b3 = p2;

    float u = 1.0f - t;
    return u*u*u*b0 + 3.0f*u*u*t*b1 + 3.0f*u*t*t*b2 + t*t*t*b3;
}

/// @brief Estima o comprimento do segmento atual por amostragem uniforme (10 amostras).
/// @return Comprimento aproximado do segmento em unidades de mundo (minimo 1.0).
float Trajectory::segmentLength() const
{
    const int SAMPLES = 10;
    float len = 0.0f;
    float saved = t;
    const_cast<Trajectory*>(this)->t = 0.0f;
    glm::vec3 prev = currentPos();
    for (int i = 1; i <= SAMPLES; ++i) {
        const_cast<Trajectory*>(this)->t = (float)i / SAMPLES;
        glm::vec3 cur = currentPos();
        len += glm::length(cur - prev);
        prev = cur;
    }
    const_cast<Trajectory*>(this)->t = saved;
    return (len > 1e-4f) ? len : 1.0f;
}

/// @brief Avanca o parametro t da trajetoria proporcionalmente ao delta time e a velocidade.
/// @param dt Delta time em segundos desde o ultimo frame.
void Trajectory::update(float dt)
{
    if (!active || !hasPath()) return;
    t += speed * dt / segmentLength();
    if (t >= 1.0f) {
        t   = 0.0f;
        idx = (idx + 1) % (int)points.size();
    }
}

/**
 * @brief Carrega pontos de controle a partir de um arquivo de texto.
 * @param path Caminho para o arquivo de trajetoria.
 * @return true se ao menos um ponto foi lido com sucesso.
 */
bool Trajectory::loadFromFile(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        glm::vec3 p;
        if (ss >> p.x >> p.y >> p.z) points.push_back(p);
    }
    return !points.empty();
}

/**
 * @brief Salva os pontos de controle atuais em um arquivo de texto.
 * @param path Caminho de destino para o arquivo de trajetoria.
 */
void Trajectory::saveToFile(const std::string& path) const
{
    std::ofstream f(path);
    f << "# trajetoria - " << points.size() << " pontos\n";
    for (const auto& p : points)
        f << p.x << " " << p.y << " " << p.z << "\n";
}
