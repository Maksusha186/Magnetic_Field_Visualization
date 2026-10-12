#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

class Shader {
public:
    unsigned int ID;

    Shader();
    ~Shader();

    // Запрещаем копирование во избежание двойного удаления glDeleteProgram
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Поддержка перемещения (Move semantics)
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    // Компиляция из исходного кода в строках
    bool compileFromSource(const char* vertexSource, const char* fragmentSource);

    // Загрузка и компиляция из файлов на диске
    bool loadFromFiles(const char* vertexPath, const char* fragmentPath);

    // Активация шейдера (glUseProgram)
    void use() const;

    // -----------------------------------------------------------------
    // Утилиты для установки Uniform-переменных
    // -----------------------------------------------------------------
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec2(const std::string& name, float x, float y) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec3(const std::string& name, float x, float y, float z) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setVec4(const std::string& name, float x, float y, float z, float w) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;

private:
    void checkCompileErrors(unsigned int shader, const std::string& type);
};

// ---------------------------------------------------------------------
// Встроенные GLSL Шейдеры из оригинального main.cpp
// ---------------------------------------------------------------------
namespace ShaderSources {

    // Шейдер для SOLENOID (поверхности, каркас, оси, линии)
    inline const char* SOLID_VERTEX = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
    )";

    inline const char* SOLID_FRAGMENT = R"(
    #version 330 core
    out vec4 FragColor;
    uniform vec3 color;
    void main() {
        FragColor = vec4(color, 1.0);
    }
    )";

    // Шейдер для SLICE (текстурированный градиент с изолиниями)
    inline const char* SLICE_VERTEX = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;
    out vec2 TexCoord;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        TexCoord = aTexCoord;
    }
    )";

    inline const char* SLICE_FRAGMENT = R"(
    #version 330 core
    in vec2 TexCoord;
    out vec4 FragColor;
    uniform sampler2D fieldTexture;
    uniform float brightness;
    uniform float contrast;

    vec3 colorFromValue(float t) {
        if (t < 0.25) return mix(vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0), t * 4.0);
        if (t < 0.50) return mix(vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 0.0), (t - 0.25) * 4.0);
        if (t < 0.75) return mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), (t - 0.50) * 4.0);
        return mix(vec3(1.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), (t - 0.75) * 4.0);
    }

    void main() {
        float t = texture(fieldTexture, TexCoord).r;

        // 1. Считаем изолинии
        float isolines = fract(t * 30.0); 
        float lineInclusion = smoothstep(0.0, 0.1, isolines) - smoothstep(0.9, 1.0, isolines);
        
        // 2. Базовый цвет с учетом изолиний
        vec3 baseColor = colorFromValue(t) * (0.8 + 0.2 * lineInclusion);

        // 3. Применяем контраст и яркость
        vec3 color = (baseColor - 0.5) * contrast + 0.5;
        color = color + (brightness - 1.0);

        FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
    }
    )";

} // namespace ShaderSources

#endif // SHADER_H
