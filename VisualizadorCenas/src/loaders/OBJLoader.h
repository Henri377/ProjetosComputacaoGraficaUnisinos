#pragma once
#include "SceneObject.h"
#include <string>
#include <vector>

/**
 * @brief Retorna o diretorio contido em um caminho de arquivo.
 * @param path Caminho completo do arquivo (pode usar '/' ou '\\').
 * @return String com o diretorio terminado em separador, ou "" se nao houver diretorio.
 */
std::string dirOf(const std::string& path);

/**
 * @brief Resolve um caminho relativo a partir de um diretorio base.
 * @param base Diretorio base (geralmente obtido via dirOf).
 * @param rel  Caminho relativo ou absoluto a ser resolvido.
 * @return Caminho absoluto resultante, ou "" se rel estiver vazio.
 */
std::string resolvePath(const std::string& base, const std::string& rel);

/**
 * @brief Carrega um arquivo OBJ e popula a lista de SubMeshes.
 *
 * Faz parse de vertices, coordenadas de textura, normais e faces (com
 * triangulacao por fan). Carrega automaticamente o arquivo MTL referenciado,
 * se existir, e aplica os materiais e texturas a cada grupo.
 *
 * @param objPath    Caminho absoluto ou relativo ao arquivo .obj.
 * @param out        Vetor onde as SubMeshes geradas serao adicionadas.
 * @param fallbackTex Textura usada quando o MTL nao especifica map_Kd.
 * @return true se ao menos uma SubMesh foi carregada com sucesso.
 */
bool loadOBJ(const std::string& objPath, std::vector<SubMesh>& out,
             const std::string& fallbackTex = "");
