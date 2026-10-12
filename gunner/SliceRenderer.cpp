#include "SliceRenderer.h"
#include <iostream>

SliceRenderer::SliceRenderer() : m_sliceVAO(0), m_sliceVBO(0), m_sliceEBO(0), m_sliceTexture(0) {}

SliceRenderer::~SliceRenderer() {
    cleanup();
}

void SliceRenderer::init() {
    if (m_sliceVAO != 0) return;

    float y_min = -12.0f, y_max = 12.0f;
    float z_min = -12.0f, z_max = 12.0f;

    float vertices[] = {
        // Позиции (X, Y, Z)     // Текстурные координаты (U, V)
        0.0f, y_min, z_min,       0.0f, 0.0f, // Bottom-Left
        0.0f, y_min, z_max,       1.0f, 0.0f, // Bottom-Right
        0.0f, y_max, z_max,       1.0f, 1.0f, // Top-Right
        0.0f, y_max, z_min,       0.0f, 1.0f  // Top-Left
    };

    unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };

    glGenVertexArrays(1, &m_sliceVAO);
    glGenBuffers(1, &m_sliceVBO);
    glGenBuffers(1, &m_sliceEBO);

    glBindVertexArray(m_sliceVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_sliceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_sliceEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    glGenTextures(1, &m_sliceTexture);
    glBindTexture(GL_TEXTURE_2D, m_sliceTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    float clearColor = 0.0f;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, 1, 1, 0, GL_RED, GL_FLOAT, &clearColor);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void SliceRenderer::cleanup() {
    if (m_sliceVAO != 0) {
        glDeleteVertexArrays(1, &m_sliceVAO);
        m_sliceVAO = 0;
    }
    if (m_sliceVBO != 0) {
        glDeleteBuffers(1, &m_sliceVBO);
        m_sliceVBO = 0;
    }
    if (m_sliceEBO != 0) {
        glDeleteBuffers(1, &m_sliceEBO);
        m_sliceEBO = 0;
    }
    if (m_sliceTexture != 0) {
        glDeleteTextures(1, &m_sliceTexture);
        m_sliceTexture = 0;
    }
}

void SliceRenderer::startCalculation(AppState& state, const CentralSolenoid& solenoid,
    float xPlane, float yMin, float yMax, float zMin, float zMax) {

    state.stopCalculation = false;
    state.calculationProgress = 0.0f;
    state.isCalculating = true;

    int w = 512;
    int h = 512;
    state.textureWidth = w;
    state.textureHeight = h;
    state.fieldData.assign(w * h, 0.0f);

    std::vector<CSModuleInfo> modulesCopy = solenoid.getModules();

    state.calcThread = std::async(std::launch::async, computeSliceTask,
        modulesCopy,
        std::ref(state.fieldData),
        std::ref(state.calculationProgress),
        std::ref(state.stopCalculation),
        w, h, xPlane, yMin, yMax, zMin, zMax
    );
}

void SliceRenderer::computeSliceTask(
    std::vector<CSModuleInfo> modules,
    std::vector<float>& targetFieldData,
    std::atomic<float>& progress,
    std::atomic<bool>& stopFlag,
    int width, int height,
    float x_plane, float y_min, float y_max, float z_min, float z_max)
{
    int w = width;
    int h = height;
    std::vector<float> raw_B(w * h, 0.0f);

    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;

    std::vector<std::future<void>> futures;
    std::atomic<int> rowsFinished(0);

    for (unsigned int t = 0; t < numThreads; ++t) {
        futures.push_back(std::async(std::launch::async, [&, t]() {
            for (int i = t; i < w; i += numThreads) {
                if (stopFlag) return;

                float z = z_min + (z_max - z_min) * (float)i / (float)(w - 1);
                for (int j = 0; j < h; j++) {
                    float y = y_min + (y_max - y_min) * (float)j / (float)(h - 1);
                    glm::vec3 p(x_plane, y, z);

                    glm::vec3 Bvec(0.0f);
                    for (const auto& mod : modules) {
                        if (!mod.isCalculationEnabled) continue;
                        Bvec += MagneticField::fieldFromCSModuleNumerical(
                            p, mod.yCenter, mod.height,
                            mod.innerRadius, mod.outerRadius,
                            mod.current * (float)mod.turns, 16, 32
                        );
                    }
                    raw_B[j * w + i] = log10f(glm::length(Bvec) + 1.0f);
                }
                rowsFinished++;
                progress = (float)rowsFinished / (float)w;
            }
            }));
    }

    for (auto& f : futures) f.get();
    if (stopFlag) return;

    float B_min = *std::min_element(raw_B.begin(), raw_B.end());
    float B_max = *std::max_element(raw_B.begin(), raw_B.end());
    float range = B_max - B_min;

    for (int k = 0; k < w * h; k++) {
        float norm = (range > 1e-9f) ? (raw_B[k] - B_min) / range : 0.0f;
        targetFieldData[k] = powf(norm, 0.6f);
    }
}

void SliceRenderer::checkAsyncResults(AppState& state) {
    if (state.isCalculating && state.calcThread.valid() &&
        state.calcThread.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {

        state.isCalculating = false;

        if (!state.stopCalculation) {
            uploadTextureData(state);
        }
    }
}

void SliceRenderer::uploadTextureData(AppState& state) {
    if (m_sliceTexture == 0 || state.fieldData.empty()) return;

    glBindTexture(GL_TEXTURE_2D, m_sliceTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, state.textureWidth, state.textureHeight, 0, GL_RED, GL_FLOAT, state.fieldData.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    state.textureValid = true;
}

void SliceRenderer::draw(const Shader& sliceShader, const glm::mat4& view, const glm::mat4& projection, const AppState& state) {
    if (!state.textureValid || m_sliceVAO == 0) return;

    sliceShader.use();
    sliceShader.setMat4("view", view);
    sliceShader.setMat4("projection", projection);
    sliceShader.setMat4("model", glm::mat4(1.0f));
    sliceShader.setInt("fieldTexture", 0);
    sliceShader.setFloat("brightness", state.fieldBrightness);
    sliceShader.setFloat("contrast", state.fieldContrast);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_sliceTexture);

    glBindVertexArray(m_sliceVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}