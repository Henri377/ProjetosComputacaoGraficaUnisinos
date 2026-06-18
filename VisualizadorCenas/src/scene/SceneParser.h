#pragma once
#include "Light.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>

/**
 * @brief Descricao completa de uma cena lida do arquivo de configuracao.
 *
 * Agrupa os parametros iniciais da camera, a lista de luzes e a lista de
 * especificacoes de objetos a serem carregados e posicionados na cena.
 */
struct SceneDef {
    /// @brief Posicao inicial da camera no espaco do mundo.
    glm::vec3 camPos{0, 0, 5};

    /// @brief Angulo horizontal inicial da camera (graus).
    float camYaw   = -90.0f;

    /// @brief Angulo vertical inicial da camera (graus).
    float camPitch =   0.0f;

    /// @brief Campo de visao inicial da camera (graus).
    float camFov   =  45.0f;

    /// @brief Plano de corte proximo da camera.
    float camNear  =   0.1f;

    /// @brief Plano de corte distante da camera.
    float camFar   = 100.0f;

    /// @brief Lista de luzes definidas na cena.
    std::vector<Light> lights;

    /**
     * @brief Especificacao de um objeto a ser carregado e instanciado na cena.
     */
    struct ObjSpec {
        /// @brief Nome de exibicao do objeto.
        std::string name = "Objeto";

        /// @brief Caminho para o arquivo .obj (relativo ao arquivo de cena).
        std::string file;

        /// @brief Caminho para o arquivo de trajetoria (opcional).
        std::string trajectoryFile;

        /// @brief Posicao inicial do objeto no espaco do mundo.
        glm::vec3 position{0};

        /// @brief Fator de escala inicial do objeto (por eixo).
        glm::vec3 scale{1};

        /// @brief Rotacao inicial do objeto em graus (por eixo XYZ).
        glm::vec3 rotation{0};

        /// @brief Velocidade de percurso da trajetoria (unidades/segundo).
        float     trajSpeed = 0.8f;
    };

    /// @brief Lista de especificacoes de objetos definidos na cena.
    std::vector<ObjSpec> objects;
};

/**
 * @brief Faz parse de um arquivo de cena no formato INI por secoes.
 *
 * Reconhece as secoes [camera], [light] e [object], preenchendo
 * a estrutura SceneDef com os valores encontrados.
 *
 * @param path Caminho para o arquivo de cena (.txt ou similar).
 * @return SceneDef preenchida; valores padrao se o arquivo nao for encontrado.
 */
SceneDef parseScene(const std::string& path);
