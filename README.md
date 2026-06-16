# Computação Gráfica - Híbrido

Repositório de exemplos de códigos em C++ utilizando OpenGL moderna (3.3+) criado para a Atividade Acadêmica Computação Gráfica do curso de graduação em Ciência da Computação - modalidade híbrida - da Unisinos. Ele é estruturado para facilitar a organização dos arquivos e a compilação dos projetos utilizando CMake.

## 📂 Estrutura do Repositório

```plaintext
📂 CGCCHibrido/
├── 📂 include/               # Cabeçalhos e bibliotecas de terceiros
│   ├── 📂 glad/              # Cabeçalhos da GLAD (OpenGL Loader)
│   │   ├── glad.h
│   │   ├── 📂 KHR/           # Diretório com cabeçalhos da Khronos (GLAD)
│   │       ├── khrplatform.h
├── 📂 common/                # Código reutilizável entre os projetos
│   ├── glad.c                # Implementação da GLAD
├── 📂 src/                   # Código-fonte dos exemplos e exercícios
│   ├── Hello3D.cpp           # Exemplo básico de renderização com OpenGL
│   ├── ...                   # Outros exemplos e exercícios futuros
├── 📂 build/                 # Diretório gerado pelo CMake (não incluído no repositório)
├── 📂 assets/                # diretório com modelos 3D, texturas, fontes etc
├── 📄 CMakeLists.txt         # Configuração do CMake para compilar os projetos
├── 📄 README.md              # Este arquivo, com a documentação do repositório
├── 📄 GettingStarted.md      # Tutorial detalhado sobre como compilar usando o CMake
```

Siga as instruções detalhadas em [GettingStarted.md](GettingStarted.md) para configurar e compilar o projeto.

## ⚠️ **IMPORTANTE: Baixar a GLAD Manualmente**
Para que o projeto funcione corretamente, é necessário **baixar a GLAD manualmente** utilizando o **GLAD Generator**.

### 🔗 **Acesse o web service do GLAD**:
👉 [GLAD Generator](https://glad.dav1d.de/)

### ⚙️ **Configuração necessária:**
- **API:** OpenGL  
- **Version:** 3.3+ (ou superior compatível com sua máquina)  
- **Profile:** Core  
- **Language:** C/C++  

### 📥 **Baixe e extraia os arquivos:**
Após a geração, extraia os arquivos baixados e coloque-os nos diretórios correspondentes:
- Copie **`glad.h`** para `include/glad/`
- Copie **`khrplatform.h`** para `include/glad/KHR/`
- Copie **`glad.c`** para `common/`

🚨 **Sem esses arquivos, a compilação falhará!** É necessário colocar esses arquivos nos diretórios corretos, conforme a orientação acima.

---

## 🗂️ Exemplos disponíveis

| Executável | Arquivo | Descrição |
|---|---|---|
| `Hello3D` | `Hello3D.cpp` | Renderização básica com OpenGL |
| `TriangleTex` | `TriangleTex.cpp` | Triângulo com textura |
| `SpherePhong` | `SpherePhong.cpp` | Esfera com iluminação Phong |
| `Vivencial` | `Vivencial.cpp` | Cena com múltiplos OBJs e transformações |
| `Texturas` | `Texturas.cpp` | Carregamento e aplicação de texturas |
| `PhongTexturas` | `PhongTexturas.cpp` | Phong + texturas |
| `PhongTexturasCamera` | `PhongTexturasCamera.cpp` | Phong + texturas + câmera FPS livre |
| `CurvasParametricas` | `CurvasParametricas.cpp` | Trajetórias paramétricas cíclicas para objetos 3D |

---

## 🎯 CurvasParametricas — Trajetórias Paramétricas

Baseado em `PhongTexturasCamera`, adiciona um sistema de **trajetórias cíclicas** para cada objeto da cena. Os pontos de controle são gravados interativamente via teclado (ou carregados de arquivo) e o objeto percorre os pontos em loop com **interpolação linear (LERP)**.

### Como funciona

Cada objeto possui uma `Trajectory` com:
- Uma lista de pontos de controle `(x, y, z)`
- Um parâmetro `t ∈ [0, 1)` que percorre o segmento atual
- Ao atingir o último ponto, retorna automaticamente ao primeiro (**comportamento cíclico**)

A posição interpolada usa `glm::mix(P_i, P_{i+1}, t)`.

### Controles

| Tecla | Ação |
|---|---|
| `1` / `2` | Seleciona objeto (Suzanne / Cubo) |
| `F` | Adiciona a posição atual do objeto como novo ponto de controle |
| `T` | Ativa / desativa a animação de trajetória do objeto selecionado |
| `C` | **Reinicia** — apaga todos os pontos e para o movimento |
| `M` | Salva a trajetória em `assets/trajetoria_N.txt` |
| Setas | Move o objeto (X/Y) — apenas com trajetória inativa |
| `PgUp` / `PgDn` | Move o objeto (Z) — apenas com trajetória inativa |
| `+` / `-` (numpad) | Escala uniforme |
| `X` / `Y` / `Z` | Rotação automática no eixo escolhido |
| `WASD` | Move a câmera (FPS) |
| Mouse | Orienta a câmera |
| Scroll | Zoom (altera FOV) |
| `ESC` | Sai |

### Como testar

```bash
cd build
cmake --build . --target CurvasParametricas
./CurvasParametricas      # Linux/Mac
CurvasParametricas.exe    # Windows
```

**Fluxo básico:**

1. Pressione `1` para selecionar a Suzanne
2. Use as **setas** para posicioná-la em um ponto desejado
3. Pressione `F` para gravar o ponto
4. Mova para outra posição e pressione `F` novamente (repita para quantos pontos quiser)
5. Pressione `T` para ativar a trajetória — o objeto começa a se mover em ciclo
6. Pressione `M` para salvar os pontos (serão recarregados na próxima execução)
7. Pressione `C` para reiniciar os pontos e redefinir a trajetória

### Arquivo de trajetória

Formato simples — uma posição `x y z` por linha; linhas com `#` são ignoradas:

```
# trajetoria_0.txt — Suzanne
-0.8  0.0  0.0
 0.0  0.8  0.0
 0.8  0.0  0.0
 0.0 -0.8  0.0
```

Arquivos salvos em:
- `assets/trajetoria_0.txt` — Suzanne
- `assets/trajetoria_1.txt` — Cubo

