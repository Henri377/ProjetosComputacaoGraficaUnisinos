#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <string>

/**
 * @brief Trajetoria parametrica suave baseada em splines Catmull-Rom.
 *
 * Armazena uma sequencia de pontos de controle e interpola posicoes
 * ao longo do caminho usando cubicas de Bezier derivadas de Catmull-Rom.
 * A trajetoria e percorrida em loop continuo quando ativa.
 */
struct Trajectory {
    /// @brief Lista de pontos de controle da trajetoria no espaco do mundo.
    std::vector<glm::vec3> points;

    /// @brief Indice do segmento atual (entre points[idx] e points[idx+1]).
    int   idx    = 0;

    /// @brief Parametro de interpolacao no segmento atual (0.0 a 1.0).
    float t      = 0.0f;

    /// @brief Velocidade de percurso da trajetoria (unidades de mundo por segundo).
    float speed  = 0.8f;

    /// @brief Indica se a trajetoria esta ativa e deve ser atualizada a cada frame.
    bool  active = false;

    /**
     * @brief Verifica se ha pontos suficientes para formar ao menos um segmento.
     * @return true se houver 2 ou mais pontos de controle.
     */
    bool hasPath() const { return (int)points.size() >= 2; }

    /**
     * @brief Calcula a posicao interpolada atual na spline Catmull-Rom.
     * @return Ponto 3D correspondente ao parametro t no segmento idx.
     */
    glm::vec3 currentPos() const;

    /**
     * @brief Avanca a trajetoria com base no tempo decorrido.
     * @param dt Delta time em segundos desde o ultimo frame.
     */
    void      update(float dt);

    /**
     * @brief Adiciona um ponto de controle ao final da trajetoria.
     * @param p Coordenadas do novo ponto no espaco do mundo.
     */
    void      addPoint(const glm::vec3& p) { points.push_back(p); }

    /// @brief Remove todos os pontos de controle e reinicia o estado da trajetoria.
    void      clear() { points.clear(); idx = 0; t = 0.0f; active = false; }

    /**
     * @brief Carrega pontos de controle a partir de um arquivo de texto.
     *
     * Cada linha nao comentada deve conter tres valores float (x y z).
     * Linhas iniciadas por '#' sao ignoradas.
     *
     * @param path Caminho para o arquivo de trajetoria.
     * @return true se ao menos um ponto foi carregado com sucesso.
     */
    bool      loadFromFile(const std::string& path);

    /**
     * @brief Salva os pontos de controle atuais em um arquivo de texto.
     * @param path Caminho de destino para o arquivo de trajetoria.
     */
    void      saveToFile(const std::string& path) const;

private:
    /**
     * @brief Estima o comprimento do segmento atual por amostragem uniforme.
     * @return Comprimento aproximado do segmento em unidades de mundo.
     */
    float segmentLength() const;
};
