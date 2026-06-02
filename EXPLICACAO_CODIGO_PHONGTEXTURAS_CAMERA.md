# Explicacao do Codigo - PhongTexturasCamera

Este arquivo explica a implementacao de `src/PhongTexturasCamera.cpp` de forma direta, focando no que cada parte do codigo faz.

## Visao geral

O programa renderiza duas malhas (`Suzanne` e `Cube`) com:

- textura
- iluminacao Phong (ambient + diffuse + specular)
- transformacoes de objeto (translacao, escala e rotacao)
- camera livre com teclado/mouse/scroll

## Blocos principais do arquivo

## 1) Shaders (strings GLSL)

### `vertexShaderSource`

Responsavel por:

- receber atributos do vertice (`position`, `texc`, `normal`)
- transformar para tela com:
  - `model` -> mundo
  - `view` -> camera
  - `projection` -> perspectiva
- enviar dados interpolados para o fragment shader:
  - `fragTexc`
  - `fragPos`
  - `scaledNormal`

### `fragmentShaderSource`

Responsavel por:

- ler cor da textura (`texBuff`)
- calcular iluminacao Phong usando:
  - `ka`, `kd`, `ks`, `q`
  - `lightPos`, `lightColor`
  - `cameraPos` (para o especular)
- escrever a cor final em `color`

## 2) Estruturas de dados

### `Material`

Guarda parametros lidos do `.mtl`:

- `ka`, `kd`, `ks`, `q`
- `textureName`

### `Mesh`

Guarda dados de cada objeto em cena:

- recursos OpenGL (`VAO`, `textureID`)
- quantidade de vertices
- transformacoes (`position`, `scale`, `baseRotation`, `rotation`)
- parametros de material usados no shader (`ka`, `kd`, `ks`, `q`)

## 3) Variaveis globais de camera

- `cameraPos`, `cameraFront`, `cameraUp`: base da camera
- `yaw`, `pitch`: orientacao
- `fov`: zoom via scroll
- `firstMouse`, `lastX`, `lastY`: controle do primeiro evento de mouse
- `deltaTime`, `lastFrame`: velocidade independente de FPS

## 4) Funcoes utilitarias de caminho/material

### `directoryOf(...)`

Retorna pasta de um caminho de arquivo.

### `resolvePath(...)`

Converte caminho relativo em absoluto com base em um diretorio base.

### `loadMaterialFromMTL(...)`

Le arquivo `.mtl` e extrai coeficientes de material e nome da textura.

## 5) Carregamento de geometria/textura

### `loadTexturedOBJ(...)`

Le `.obj` com:

- vertices (`v`)
- coordenadas UV (`vt`)
- normais (`vn`)
- faces (`f`)

Monta um buffer intercalado `[pos(3), uv(2), normal(3)]`, cria VBO/VAO e configura atributos:

- location 0 -> posicao
- location 1 -> UV
- location 2 -> normal

### `loadTexture(...)`

Carrega imagem com `stb_image`, cria textura OpenGL 2D, define filtros/wrap e gera mipmaps.

### `loadMesh(...)`

Pipeline por malha:

1. chama `loadTexturedOBJ(...)`
2. aplica material do `.mtl`
3. carrega textura (`map_Kd` ou fallback)

## 6) Input e camera

### `processInput(...)`

Processa input continuo por frame:

- `ESC`: fecha janela
- `WASD`: movimenta camera usando `cameraFront` e vetor lateral via `cross(...)`
- velocidade com `deltaTime`

### `mouse_callback(...)`

Converte deslocamento do mouse em orientacao da camera:

- atualiza `yaw` e `pitch`
- limita `pitch` em `[-89, 89]`
- recalcula `cameraFront`
- recalcula `cameraUp` via base ortonormal (`right` e `cross`)

### `scroll_callback(...)`

Controla zoom alterando `fov` no intervalo `[1, 45]`.

## 7) Funcoes auxiliares de organizacao

### `updateCameraUniforms(...)`

Atualiza uniforms dependentes da camera:

- `view` com `glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp)`
- `projection` com `glm::perspective(glm::radians(fov), aspect, 0.1, 100.0)`
- `cameraPos` no shader

### `updateActiveMeshRotation()`

Aplica rotacao automatica no eixo selecionado (`X`, `Y`, `Z`) para a malha ativa.

### `handleMeshTransformKeys(...)`

Isola logica de translacao/escala da malha ativa:

- translacao: setas e `PageUp/PageDown`
- escala uniforme: `Numpad +/-`
- escala por eixo: `I/K`, `O/L`, `P/;`

### `key_callback(...)`

Processa eventos discretos de teclado:

- seleciona malha (`1` e `2`)
- seleciona eixo de rotacao automatica (`X`, `Y`, `Z`)
- delega transformacoes para `handleMeshTransformKeys(...)`

## 8) `main()`: fluxo da aplicacao

Ordem geral:

1. inicializa GLFW e janela
2. registra callbacks (teclado, mouse, scroll)
3. ativa cursor desabilitado para camera FPS
4. inicializa GLAD e viewport
5. compila shader e seta `texBuff`
6. carrega `Suzanne` e `Cube`
7. define luz
8. pega locations de uniforms
9. loop principal:
   - atualiza `deltaTime`
   - processa input/poll de eventos
   - limpa buffers
   - atualiza uniforms de camera
   - atualiza rotacao automatica
   - desenha malhas
   - swap buffers
10. libera recursos OpenGL e encerra GLFW


