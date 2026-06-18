#pragma once
#include <string>

/**
 * @brief Propriedades de material para iluminacao Phong e texturizacao.
 *
 * Armazena os coeficientes de reflexao ambiente, difusa e especular,
 * o expoente de brilho (shininess) e o caminho para a textura difusa.
 */
struct Material {
    /// @brief Coeficiente de reflexao ambiente (ka).
    float  ka        = 0.2f;

    /// @brief Coeficiente de reflexao difusa (kd).
    float  kd        = 0.7f;

    /// @brief Coeficiente de reflexao especular (ks).
    float  ks        = 0.5f;

    /// @brief Expoente de brilho especular (shininess).
    float  shininess = 32.0f;

    /// @brief Caminho para o arquivo de textura difusa (map_Kd). Vazio = sem textura.
    std::string texPath;
};
