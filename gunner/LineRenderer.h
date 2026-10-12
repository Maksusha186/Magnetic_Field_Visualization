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

    LineRenderer(const LineRenderer&) = delete;
    LineRenderer& operator=(const LineRenderer&) = delete;

    void init();

    void cleanup();

    void startCalculation(AppState& state, const CentralSolenoid& solenoid);

    void checkAsyncResults(AppState& state);

    void uploadBufferData(AppState& state);

    void draw(const Shader& solidShader, const glm::mat4& view, const glm::mat4& projection, const AppState& state);

private:
    unsigned int m_linesVAO = 0;
    unsigned int m_linesVBO = 0;
    unsigned int m_arrowsVAO = 0;
    unsigned int m_arrowsVBO = 0;

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