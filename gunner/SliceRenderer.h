#ifndef SLICE_RENDERER_H
#define SLICE_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>
#include <future>
#include <atomic>
#include <algorithm>
#include <thread>

#include "AppState.h"
#include "Shader.h"
#include "CentralSolenoid.h"
#include "physics.h"

class SliceRenderer {
public:
    SliceRenderer();
    ~SliceRenderer();

    // Запрет копирования
    SliceRenderer(const SliceRenderer&) = delete;
    SliceRenderer& operator=(const SliceRenderer&) = delete;

    // Инициализация OpenGL буферов и текстуры
    void init();

    // Освобождение GPU ресурсов
    void cleanup();

    // Запуск асинхронного расчета среза в фоновом потоке
    void startCalculation(AppState& state, const CentralSolenoid& solenoid,
        float xPlane = 0.0f, float yMin = -12.0f, float yMax = 12.0f,
        float zMin = -12.0f, float zMax = 12.0f);

    // Проверка статуса фонового потока (вызывается в главном цикле)
    void checkAsyncResults(AppState& state);

    // Прямая загрузка сырого буфера в GPU (например, после чтения .field из проекта)
    void uploadTextureData(AppState& state);

    // Отрисовка плоскости среза
    void draw(const Shader& sliceShader, const glm::mat4& view, const glm::mat4& projection, const AppState& state);

    unsigned int getTextureID() const { return m_sliceTexture; }

private:
    unsigned int m_sliceVAO = 0;
    unsigned int m_sliceVBO = 0;
    unsigned int m_sliceEBO = 0;
    unsigned int m_sliceTexture = 0;

    // Внутренняя функция фонового многопоточного расчета сетки поля
    static void computeSliceTask(
        std::vector<CSModuleInfo> modules,
        std::vector<float>& targetFieldData,
        std::atomic<float>& progress,
        std::atomic<bool>& stopFlag,
        int width, int height,
        float x_plane, float y_min, float y_max, float z_min, float z_max
    );
};

#endif // SLICE_RENDERER_H