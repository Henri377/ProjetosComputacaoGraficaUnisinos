# Visualizador de Cenas 3D

Visualizador interativo em OpenGL 4.5 com suporte a iluminação Phong, carregamento de modelos OBJ, câmera FPS e animação por trajetórias Catmull-Rom.

---

## Build

**Pré-requisitos:** CMake 3.10+, compilador C++17, drivers OpenGL 4.5.

```bash
# Na raiz do projeto VisualizadorCenas/
cmake -B build
cmake --build build
```

As dependências (GLFW, GLM, stb_image) são baixadas automaticamente via `FetchContent`.

O executável gerado fica em `build/VisualizadorCenas` (Linux/macOS) ou `build\Debug\VisualizadorCenas.exe` (Windows).

---

## Executando

```bash
./build/VisualizadorCenas
# ou passando uma cena customizada:
./build/VisualizadorCenas assets/scene.txt
```

Se nenhum arquivo for passado, o programa busca `assets/scene.txt` relativo ao executável.

---

## Formato da Cena (`scene.txt`)

Arquivo INI com seções `[camera]`, `[light]` e `[object]`. Linhas começando com `#` são comentários.

```ini
[camera]
position  0.0  1.0  8.0
yaw      -90.0
pitch      0.0
fov       45.0
near       0.1
far      100.0

[light]
position  2.0  4.0  5.0
color     1.0  1.0  1.0
intensity 1.0

[light]
position -3.0  1.0  2.0
color     0.4  0.4  0.6
intensity 0.4

[object]
name      Skater
file      Modelos3D/Skater.obj
position  0.0  0.0  0.0
scale     0.1
rotation  0.0 180.0 0.0
trajectory trajetoria_skater.txt
speed     1.5

[object]
name      Ball
file      Modelos3D/Ball.obj
scale     0.04
trajectory trajetoria_ball.txt
speed     2.0
```

Caminhos de `file` e `trajectory` são relativos ao diretório do arquivo de cena.

---

## Features

### Camera FPS
| Ação | Controle |
|---|---|
| Mover | `W A S D` |
| Olhar | Mouse (arrastar) |
| Zoom (FOV) | Scroll do mouse |

### Seleção e Transformação de Objetos
| Ação | Controle |
|---|---|
| Selecionar objeto | `1` – `9` |
| Rotacionar em X/Y/Z | `X` / `Y` / `Z` |
| Parar rotação | `P` |
| Mover (sem trajetória) | Setas + `PgUp` / `PgDn` |
| Escalar (sem trajetória) | `Numpad +` / `Numpad -` |

### Trajetórias Catmull-Rom
Cada objeto pode seguir uma curva suave definida por pontos de controle. A interpolação é feita com splines Catmull-Rom com velocidade constante em unidades/segundo.

| Ação | Controle |
|---|---|
| Adicionar ponto na posição atual | `F` |
| Ativar/desativar trajetória | `T` |
| Limpar trajetória | `C` |
| Salvar trajetória em arquivo | `M` |

Arquivo de trajetória: uma linha por ponto `x y z`, comentários com `#`.

```
# trajetoria_skater.txt
0.0  0.0  0.0
2.0  0.0  1.0
4.0  0.0  0.0
```

### Iluminação
- Modelo **Phong** (ambiente + difuso + especular) com até **4 luzes** simultâneas.
- Componente difuso modulado pela textura do objeto.

| Ação | Controle |
|---|---|
| Toggle luz key | `[` |
| Toggle luz fill | `]` |
| Toggle luz back | `\` |
| Aumentar intensidade | `I` (segurar) |
| Diminuir intensidade | `O` (segurar) |

### Carregamento de Modelos
- Parser completo de **OBJ + MTL**.
- Suporte a múltiplos grupos de materiais por arquivo.
- Triangulação automática de polígonos (método fan).
- Texturas PNG/JPG/BMP via stb_image; fallback cinza se arquivo ausente.
- Layout de vértice intercalado: `posição (3) + UV (2) + normal (3)`.

---

## Estrutura do Projeto

```
VisualizadorCenas/
├── assets/
│   ├── scene.txt              # Cena principal
│   ├── Modelos3D/             # Arquivos .obj/.mtl e texturas
│   └── trajetoria_*.txt       # Pontos de trajetória
├── src/
│   ├── main.cpp
│   ├── camera/Camera.h        # Câmera FPS (header-only)
│   ├── input/                 # Callbacks GLFW e estado global
│   ├── rendering/             # Shader, Texture2D, Material
│   ├── scene/                 # SceneParser, SceneObject, Light, Trajectory
│   └── loaders/               # OBJLoader
├── include/glad/              # Loader OpenGL
├── common/glad.c
└── CMakeLists.txt
```

---

## Dependências

| Biblioteca | Versão | Uso |
|---|---|---|
| GLFW | 3.4 | Janela e input |
| GLM | master | Matemática (vec3, mat4) |
| stb_image | master | Carregamento de texturas |
| OpenGL | 4.5 core | API gráfica |
| GLAD | local | Loader de funções GL |

Todas baixadas automaticamente pelo CMake, exceto GLAD (inclusa no repositório).


Assets:

https://www.turbosquid.com/3d-models/stylized-roller-skates-model-2559716
https://www.turbosquid.com/3d-models/vintage-leather-ball-model-2547363