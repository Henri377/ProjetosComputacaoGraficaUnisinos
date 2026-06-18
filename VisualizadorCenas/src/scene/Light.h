#pragma once
#include <glm/glm.hpp>

/**
 * @brief Representa uma fonte de luz pontual na cena.
 *
 * Contem posicao, cor, intensidade e estado de ativacao da luz.
 * Suporta ate 4 luzes simultaneas no shader (arrays lightPos/lightColor/lightIntensity).
 */
struct Light {
    /// @brief Posicao da luz no espaco do mundo.
    glm::vec3 position{2, 3, 5};

    /// @brief Cor da luz em RGB (valores normalizados entre 0 e 1).
    glm::vec3 color{1, 1, 1};

    /// @brief Intensidade da luz (multiplicador aplicado sobre a cor).
    float     intensity = 1.0f;

    /// @brief Indica se a luz esta ativa e deve ser enviada ao shader.
    bool      enabled   = true;
};
