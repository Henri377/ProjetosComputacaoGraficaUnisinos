# PhongTexturasCamera - Explicacao e Controles

Este documento descreve o que foi adicionado no arquivo `src/PhongTexturasCamera.cpp` com foco na feature de camera, mantendo as demais features do `PhongTexturas` (iluminacao Phong, textura, transformacoes e selecao de malha).

## 1) Pipeline de transformacao (Model, View, Projection)

No vertex shader, a ordem usada e:

`gl_Position = projection * view * model * vec4(position, 1.0);`

- `model`: leva vertices do espaco local do objeto para o mundo.
- `view`: aplica a transformacao da camera (posicao e orientacao).
- `projection`: projeta para perspectiva (FOV, aspect ratio, near/far).

## 2) Shader de iluminacao (Phong + textura)

O fragment shader calcula:

- **Ambient**: `ka * lightColor`
- **Diffuse**: `kd * max(dot(N, L), 0.0) * lightColor`
- **Specular**: `ks * pow(max(dot(R, V), 0.0), q) * lightColor`

Onde:

- `N` = normal normalizada
- `L` = vetor da luz para o fragmento
- `V` = vetor da camera para o fragmento
- `R` = reflexao de `-L` em relacao a `N`
- `q` = expoente especular (shininess)

A cor final:

`result = (ambient + diffuse) * objectColor + specular`

`objectColor` vem de `texture(texBuff, fragTexc).rgb`.

## 3) Camera FPS adicionada

Variaveis principais:

- `cameraPos`: posicao da camera
- `cameraFront`: direcao para onde a camera olha/anda
- `cameraUp`: vetor para cima da camera
- `yaw`, `pitch`: angulos de orientacao
- `fov`: campo de visao (zoom)
- `deltaTime`: tempo entre frames (velocidade consistente)

### 3.1 Movimento (WASD)

No `processInput()`:

- `W`: avanca (`cameraPos += cameraSpeed * cameraFront`)
- `S`: recua
- `A`: strafe esquerda (usa `cross(cameraFront, cameraUp)`)
- `D`: strafe direita

Velocidade:

`cameraSpeed = 2.5f * deltaTime`

Isso evita diferenca de velocidade entre FPS alto/baixo.

### 3.2 Mouse look (yaw/pitch)

No `mouse_callback()`:

1. Calcula deslocamento do mouse (`xoffset`, `yoffset`)
2. Aplica sensibilidade
3. Atualiza `yaw` e `pitch`
4. Faz clamp de `pitch` em `[-89, 89]`
5. Recalcula `cameraFront`
6. Recalcula base ortonormal (`right` e `cameraUp`)

Cursor desabilitado:

`glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);`

### 3.3 Zoom (scroll)

No `scroll_callback()`:

- Scroll altera `fov`
- Clamp entre `1.0f` e `45.0f`

No loop principal:

`projection = glm::perspective(glm::radians(fov), aspect, 0.1f, 100.0f);`

## 4) Atualizacao da view/projection em runtime

No loop principal:

- `view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);`
- `projection` e recalculada com `fov` e aspect ratio atual do framebuffer.
- `cameraPos` e reenviado para shader (especular depende da camera).

## 5) Mapa de controles (atual)

### Camera

- `W` / `A` / `S` / `D`: mover camera
- Mouse: olhar ao redor
- Scroll: zoom in/out (altera FOV)
- `ESC`: fechar janela

### Selecao e animacao de malha

- `1`: selecionar Suzanne
- `2`: selecionar Cube
- `X`: rotacao automatica no eixo X da malha ativa
- `Y`: rotacao automatica no eixo Y da malha ativa
- `Z`: rotacao automatica no eixo Z da malha ativa

### Transformacoes da malha ativa

- Setas (`UP`, `DOWN`, `LEFT`, `RIGHT`): translacao em X/Y
- `PageUp` / `PageDown`: translacao em Z
- `Numpad +` / `Numpad -`: escala uniforme
- `I` / `K`: escala em X (+/-)
- `O` / `L`: escala em Y (+/-)
- `P` / `;`: escala em Z (+/-)

## 6) Build do exemplo novo

Foi adicionado no CMake o target:

- `PhongTexturasCamera`

Comando:

`cmake --build build --target PhongTexturasCamera`

