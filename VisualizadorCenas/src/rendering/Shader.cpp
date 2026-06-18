#include "Shader.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>

static const char* VERT_SRC = R"glsl(
#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNorm;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

out vec3 fragPos;
out vec2 fragUV;
out vec3 fragNorm;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    gl_Position   = projection * view * worldPos;
    fragPos  = vec3(worldPos);
    fragNorm = normalMatrix * aNorm;
    fragUV   = aUV;
}
)glsl";

static const char* FRAG_SRC = R"glsl(
#version 450 core
in vec3 fragPos;
in vec2 fragUV;
in vec3 fragNorm;
out vec4 outColor;

uniform sampler2D texBuff;
uniform vec3  cameraPos;
uniform float ka;
uniform float kd;
uniform float ks;
uniform float shininess;
uniform int   numLights;
uniform vec3  lightPos[4];
uniform vec3  lightColor[4];
uniform float lightIntensity[4];

void main() {
    vec3 texColor = texture(texBuff, fragUV).rgb;
    vec3 N = normalize(fragNorm);
    vec3 V = normalize(cameraPos - fragPos);

    vec3 result = vec3(0.0);
    for (int i = 0; i < numLights && i < 4; i++) {
        vec3 L  = normalize(lightPos[i] - fragPos);
        vec3 R  = reflect(-L, N);
        vec3 lc = lightColor[i] * lightIntensity[i];

        vec3 ambient  = ka * lc;
        vec3 diffuse  = kd * max(dot(N, L), 0.0) * lc;
        vec3 specular = ks * pow(max(dot(R, V), 0.0), shininess) * lc;

        result += (ambient + diffuse) * texColor + specular;
    }
    outColor = vec4(result, 1.0);
}
)glsl";

/**
 * @brief Compila um unico estagio de shader (vertex ou fragment).
 * @param type Tipo do shader (ex.: GL_VERTEX_SHADER, GL_FRAGMENT_SHADER).
 * @param src  Codigo fonte GLSL em formato de string C.
 * @return Identificador OpenGL do shader compilado.
 */
static GLuint compileStage(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetShaderInfoLog(s, 1024, nullptr, log);
        std::cerr << "Shader error:\n" << log << "\n";
    }
    return s;
}

/// @brief Compila vertex e fragment shaders embutidos e linka o programa OpenGL.
/// @return Identificador OpenGL do programa de shader pronto para uso.
GLuint compileShaders()
{
    GLuint vs   = compileStage(GL_VERTEX_SHADER,   VERT_SRC);
    GLuint fs   = compileStage(GL_FRAGMENT_SHADER, FRAG_SRC);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { char log[1024]; glGetProgramInfoLog(prog, 1024, nullptr, log); std::cerr << log << "\n"; }
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

/**
 * @brief Envia os dados das luzes ativas para os uniforms do shader.
 * @param shader Identificador OpenGL do programa de shader ativo.
 * @param lights Vetor de luzes da cena (habilitadas e desabilitadas).
 */
void uploadLights(GLuint shader, const std::vector<Light>& lights)
{
    static const int MAX_LIGHTS = 4;
    glUseProgram(shader);
    int slot = 0;
    for (int i = 0; i < (int)lights.size() && slot < MAX_LIGHTS; ++i) {
        if (!lights[i].enabled) continue;
        std::string pi = "lightPos["       + std::to_string(slot) + "]";
        std::string ci = "lightColor["     + std::to_string(slot) + "]";
        std::string ii = "lightIntensity[" + std::to_string(slot) + "]";
        glUniform3fv(glGetUniformLocation(shader, pi.c_str()), 1, glm::value_ptr(lights[i].position));
        glUniform3fv(glGetUniformLocation(shader, ci.c_str()), 1, glm::value_ptr(lights[i].color));
        glUniform1f (glGetUniformLocation(shader, ii.c_str()),    lights[i].intensity);
        ++slot;
    }
    glUniform1i(glGetUniformLocation(shader, "numLights"), slot);
}
