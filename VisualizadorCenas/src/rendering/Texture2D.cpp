#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "Texture2D.h"
#include <iostream>

/// @brief Carrega uma imagem do disco, envia para a GPU e gera mipmaps.
/// @param path Caminho para o arquivo de imagem.
/// @return true sempre; usa pixel cinza como fallback se o arquivo nao for encontrado.
bool Texture2D::load(const std::string& path)
{
    release();
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int w, h, ch;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 0);
    if (!data) {
        std::cerr << "  Aviso: textura nao encontrada: " << path << "\n";
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

/// @brief Ativa a textura na unidade de textura especificada.
/// @param unit Unidade de textura OpenGL (ex.: GL_TEXTURE0).
void Texture2D::bind(GLenum unit) const
{
    glActiveTexture(unit);
    glBindTexture(GL_TEXTURE_2D, id);
}

/// @brief Libera o objeto de textura da GPU e zera o identificador.
void Texture2D::release()
{
    if (id) { glDeleteTextures(1, &id); id = 0; }
}
