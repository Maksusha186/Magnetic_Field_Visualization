#include "LineRenderer.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

LineRenderer::LineRenderer() : m_linesVAO(0), m_linesVBO(0), m_arrowsVAO(0), m_arrowsVBO(0) {}

LineRenderer::~LineRenderer() {
    cleanup();
}

void LineRenderer::init() {
    if (m_linesVAO != 0) return; // Уже инициализировано

    // Буфер для самих силовых линий
    glGenVertexArrays(1, &m_linesVAO);
    glGenBuffers(1, &m_linesVBO);

    glBindVertexArray(m_linesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_linesVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Буфер для стрелок направления
    glGenVertexArrays(1, &m_arrowsVAO);
    glGenBuffers(1, &m_arrowsVBO);

    glBindVertexArray(m_arrowsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_arrowsVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void LineRenderer::cleanup() {
    if (m_linesVAO != 0) {
        glDeleteVertexArrays(1, &m_linesVAO);
        m_linesVAO = 0;
    }
    if (m_linesVBO != 0) {
        glDeleteBuffers(1, &m_linesVBO);
        m_linesVBO = 0;
    }
    if (m_arrowsVAO != 0) {
        glDeleteVertexArrays(1, &m_arrowsVAO);
        m_arrowsVAO = 0;
    }
    if (m_arrowsVBO != 0) {
        glDeleteBuffers(1, &m_arrowsVBO);
        m_arrowsVBO = 0;
    }
}

void LineRenderer::startCalculation(AppState& state, const CentralSolenoid& solenoid) {
    state.stopLinesCalculation = false;
    state.linesProgress = 0.0f;
    state.isLinesCalculating = true;

    state.threadLineVertices.clear();
    state.threadArrowVertices.clear();

    // Копируем модули для потокобезопасности (Data Race Protection)
    std::vector<CSModuleInfo> modulesCopy = solenoid.getModules();

    // Запуск фонового асинхронного потока
    state.linesCalcThread = std::async(std::launch::async, computeLinesTask,
        modulesCopy,
        std::ref(state.threadLineVertices),
        std::ref(state.threadArrowVertices),
        std::ref(state.linesProgress),
        std::ref(state.stopLinesCalculation),
        state.radialDensity,
        state.angularDensity,
        state.pointsPerLine,
        state.lineStepSize,
        state.useAdaptiveStep,
        state.showLineArrows
    );
}

void LineRenderer::computeLinesTask(
    std::vector<CSModuleInfo> modules,
    std::vector<float>& outLineVertices,
    std::vector<float>& outArrowVertices,
    std::atomic<float>& progress,
    std::atomic<bool>& stopFlag,
    int rDensity, int aDensity, int maxLength,
    float userStep, bool adaptive, bool showArrows)
{
    auto fieldFuncInternal = [&](const glm::vec3& p) -> glm::vec3 {
        glm::vec3 B(0.0f);
        for (const auto& mod : modules) {
            if (!mod.isCalculationEnabled) continue;
            B += MagneticField::fieldFromCSModuleNumerical(
                p, mod.yCenter, mod.height,
                mod.innerRadius, mod.outerRadius,
                mod.current * (float)mod.turns, 16, 32
            );
        }
        return B;
        };

    int totalLines = rDensity * aDensity;
    int lineCounter = 0;
    float solenoidR = modules.empty() ? 1.3f : modules[0].innerRadius;

    for (int r = 0; r < rDensity; r++) {
        // РАСПРЕДЕЛЕНИЕ ТОЧЕК:
        // Первые слои - внутри соленоида (аксиальные линии)
        // Последние слои - снаружи (широкие дуги)
        float currentStartR = 0.05f + (solenoidR * 3.0f) * (powf((float)r / (float)rDensity, 1.5f));

        for (int a = 0; a < aDensity; a++) {
            if (stopFlag) return;

            float angle = (2.0f * M_PI * a) / (float)aDensity;
            glm::vec3 startP(currentStartR * sinf(angle), 0.1f, currentStartR * cosf(angle));

            bool loopClosed = false;

            for (int dir : {1, -1}) {
                if (loopClosed) break;

                glm::vec3 p = startP;
                float currentStep = userStep;
                glm::vec3 prevDir(0.0f);

                for (int j = 0; j < maxLength / 2; j++) {
                    glm::vec3 B = fieldFuncInternal(p);
                    float Bmag = glm::length(B);
                    if (Bmag < 1e-9f) break;

                    glm::vec3 currDir = glm::normalize(B) * (float)dir;

                    // АВТОМАТИЧЕСКИЙ ВЫБОР ШАГА
                    if (adaptive && j > 0) {
                        float angleDiff = acosf(glm::clamp(glm::dot(currDir, prevDir), -1.0f, 1.0f));
                        if (angleDiff > 0.02f) currentStep = (std::max)(0.01f, currentStep * 0.8f);
                        else if (angleDiff < 0.005f) currentStep = (std::min)(1.0f, currentStep * 1.2f);
                    }
                    prevDir = currDir;

                    glm::vec3 nextP = MagneticField::getNextLinePoint(p, currentStep * (float)dir, fieldFuncInternal);

                    float yLevel = startP.y; // 0.1f
                    // Проверка пересечения стартовой плоскости
                    bool crossedPlane = (p.y - yLevel) * (nextP.y - yLevel) < 0.0f;

                    if (j > 50 && crossedPlane) {
                        // Расстояние (X, Z) в момент пересечения
                        float dXZ = glm::distance(glm::vec2(nextP.x, nextP.z), glm::vec2(startP.x, startP.z));

                        if (dXZ < 0.5f) {
                            float t = std::abs(p.y - yLevel) / std::abs(nextP.y - p.y);
                            glm::vec3 finalP = p + t * (nextP - p);

                            outLineVertices.insert(outLineVertices.end(), { p.x, p.y, p.z, finalP.x, finalP.y, finalP.z });
                            outLineVertices.insert(outLineVertices.end(), { finalP.x, finalP.y, finalP.z, startP.x, startP.y, startP.z });

                            loopClosed = true;
                            break;
                        }
                    }

                    outLineVertices.insert(outLineVertices.end(), { p.x, p.y, p.z, nextP.x, nextP.y, nextP.z });

                    // Генерация геометрии стрелок направления
                    if (showArrows && j % 50 == 0 && j > 0) {
                        glm::vec3 B_arrow = fieldFuncInternal(p);
                        float Bmag_arrow = glm::length(B_arrow);

                        if (Bmag_arrow > 1e-8f) {
                            glm::vec3 fwd = glm::normalize(B_arrow);

                            float arrowLen = 0.10f;
                            float arrowWid = 0.05f;

                            glm::vec3 up = (std::abs(fwd.y) > 0.5f) ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
                            glm::vec3 side = glm::normalize(glm::cross(fwd, up));

                            glm::vec3 back = p - fwd * arrowLen;

                            outArrowVertices.insert(outArrowVertices.end(), {
                                p.x, p.y, p.z,
                                back.x + side.x * arrowWid, back.y + side.y * arrowWid, back.z + side.z * arrowWid,

                                p.x, p.y, p.z,
                                back.x - side.x * arrowWid, back.y - side.y * arrowWid, back.z - side.z * arrowWid
                                });
                        }
                    }

                    p = nextP;
                    if (glm::length(p) > 100.0f) break;
                }
            }
            lineCounter++;
            progress = (float)lineCounter / (float)totalLines;
        }
    }
}

void LineRenderer::checkAsyncResults(AppState& state) {
    if (state.isLinesCalculating && state.linesCalcThread.valid() &&
        state.linesCalcThread.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {

        state.isLinesCalculating = false;

        if (!state.stopLinesCalculation) {
            state.lineVertices = state.threadLineVertices;
            state.arrowVertices = state.threadArrowVertices;
            uploadBufferData(state);
        }
    }
}

void LineRenderer::uploadBufferData(AppState& state) {
    if (m_linesVBO == 0) return;

    // Загрузка вершин линий
    glBindBuffer(GL_ARRAY_BUFFER, m_linesVBO);
    glBufferData(GL_ARRAY_BUFFER, state.lineVertices.size() * sizeof(float), state.lineVertices.data(), GL_STATIC_DRAW);

    // Загрузка вершин стрелок
    glBindBuffer(GL_ARRAY_BUFFER, m_arrowsVBO);
    glBufferData(GL_ARRAY_BUFFER, state.arrowVertices.size() * sizeof(float), state.arrowVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    state.linesBufferNeedsUpdate = false;
}

void LineRenderer::draw(const Shader& solidShader, const glm::mat4& view, const glm::mat4& projection, const AppState& state) {
    if (!state.showFieldLines || state.lineVertices.empty() || m_linesVAO == 0) return;

    glDisable(GL_DEPTH_TEST);

    solidShader.use();
    solidShader.setMat4("view", view);
    solidShader.setMat4("projection", projection);
    solidShader.setMat4("model", glm::mat4(1.0f));
    solidShader.setVec3("color", glm::vec3(1.0f, 1.0f, 1.0f)); // Белый цвет линий

    glLineWidth(1.5f);

    // Отрисовка самих линий
    glBindVertexArray(m_linesVAO);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(state.lineVertices.size() / 3));

    // Отрисовка стрелок
    if (state.showLineArrows && !state.arrowVertices.empty() && m_arrowsVAO != 0) {
        glBindVertexArray(m_arrowsVAO);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(state.arrowVertices.size() / 3));
    }

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}