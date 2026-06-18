#pragma once
#include <glad/glad.h>
#include <string>

/**
 * @brief Textura 2D gerenciada via OpenGL.
 *
 * Encapsula o identificador OpenGL de uma textura 2D e oferece metodos
 * para carregamento a partir de arquivo, ativacao em uma unidade de textura
 * e liberacao dos recursos de GPU.
 */
struct Texture2D {
    /// @brief Identificador OpenGL da textura (0 = nao carregada).
    GLuint id = 0;

    /**
     * @brief Carrega uma imagem do disco e envia para a GPU.
     *
     * Utiliza stb_image para decodificar o arquivo. Caso o arquivo nao seja
     * encontrado, substitui por um pixel cinza solido. Gera mipmaps
     * automaticamente apos o upload.
     *
     * @param path Caminho para o arquivo de imagem (PNG, JPG, BMP, etc.).
     * @return true sempre (mesmo em caso de arquivo ausente, usa fallback cinza).
     */
    bool load(const std::string& path);

    /**
     * @brief Ativa a textura em uma unidade de textura OpenGL.
     * @param unit Unidade de textura alvo (ex.: GL_TEXTURE0). Padrao: GL_TEXTURE0.
     */
    void bind(GLenum unit = GL_TEXTURE0) const;

    /// @brief Libera o objeto de textura da GPU e zera o identificador.
    void release();
};
