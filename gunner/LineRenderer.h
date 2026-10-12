#ifndef LINE_RENDERER_H
#define LINE_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>
#include <future>
#include <atomic>

#include "AppState.h"
#include "Shader.h"
#include "CentralSolenoid.h"
#include "physics.h"

class LineRenderer {
public:
    LineRenderer();
    ~LineRenderer();

    // Запрет копирования
    LineRenderer(const LineRenderer&) = delete;
    LineRenderer& operator=(const LineRenderer&) = delete;

    // Инициализация VBO и VAO для линий и стрелок
    void init();

    // Освобождение GPU ресурсов
    void cleanup();

    // Запуск асинхронного расчета силовых линий (RK4) в фоновом потоке
    void startCalculation(AppState& state, const CentralSolenoid& solenoid);

    // Проверка статуса фонового потока (вызывается в главном цикле)
    void checkAsyncResults(AppState& state);

    // Прямая загрузка сырых буферов в GPU (например, после чтения .lines/.arrows из проекта)
    void uploadBufferData(AppState& state);

    // Отрисовка силовых линий и стрелок
    void draw(const Shader& solidShader, const glm::mat4& view, const glm::mat4& projection, const AppState& state);

private:
    unsigned int m_linesVAO = 0;
    unsigned int m_linesVBO = 0;
    unsigned int m_arrowsVAO = 0;
    unsigned int m_arrowsVBO = 0;

    // Внутренний статический метод фонового расчета RK4
    static void computeLinesTask(
        std::vector<CSModuleInfo> modules,
        std::vector<float>& outLineVertices,
        std::vector<float>& outArrowVertices,
        std::atomic<float>& progress,
        std::atomic<bool>& stopFlag,
        int rDensity, int aDensity, int maxLength,
        float userStep, bool adaptive, bool showArrows
    );
};

#endif // LINE_RENDERER_H