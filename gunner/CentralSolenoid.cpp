#include "CentralSolenoid.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

CentralSolenoid::CentralSolenoid() {
    generateMesh();
}

void CentralSolenoid::generateMesh() {
    // 1. —начала заполн€ем модули по умолчанию, если массив пуст
    if (m_modules.empty()) {
        for (int i = 0; i < NUM_MODULES; i++) {
            CSModuleInfo info;
            info.height = MODULE_HEIGHT;
            info.innerRadius = INNER_RADIUS;
            info.outerRadius = OUTER_RADIUS;
            info.current = CURRENT_PER_MODULE;
            info.turns = 500;
            info.isVisible = true;
            info.isCalculationEnabled = true;
            info.showProjection = true;
            m_modules.push_back(info);
        }
    }

    m_vertices.clear();
    m_indices.clear();
    m_triangleVertices.clear();
    m_triangleIndices.clear();

    const int CIRCLE_SEGMENTS = 32;

    // 2. “еперь корректно рассчитываем начальную высоту Y дл€ центрировани€
    float currentY = 0.0f;
    for (auto& m : m_modules) {
        currentY -= (m.height + 0.05f);
    }
    currentY /= 2.0f; // —мещение вниз, чтобы центр соленоида был в Y = 0

    // 3. √енераци€ геометрии и обновление координат yCenter каждого модул€
    for (int i = 0; i < static_cast<int>(m_modules.size()); i++) {
        CSModuleInfo& mod = m_modules[i];
        float yBottom = currentY;
        float yTop = currentY + mod.height;
        mod.yCenter = (yBottom + yTop) / 2.0f;

        // Wireframe геометри€
        mod.vertexStart = (int)m_vertices.size();
        for (int s = 0; s < 4; s++) {
            float r = (s < 2) ? mod.outerRadius : mod.innerRadius;
            float y = (s % 2 == 0) ? yBottom : yTop;
            for (int j = 0; j < CIRCLE_SEGMENTS; j++) {
                float a = 2.0f * (float)M_PI * j / CIRCLE_SEGMENTS;
                m_vertices.push_back(glm::vec3(r * cosf(a), y, r * sinf(a)));
            }
        }

        mod.indexStart = (int)m_indices.size();
        int bOB = mod.vertexStart, bOT = bOB + CIRCLE_SEGMENTS;
        int bIB = bOT + CIRCLE_SEGMENTS, bIT = bIB + CIRCLE_SEGMENTS;

        for (int j = 0; j < CIRCLE_SEGMENTS; j++) {
            int n = (j + 1) % CIRCLE_SEGMENTS;
            // Verticals
            m_indices.push_back(bOB + j); m_indices.push_back(bOT + j);
            m_indices.push_back(bIB + j); m_indices.push_back(bIT + j);
            // Horizontals
            m_indices.push_back(bOB + j); m_indices.push_back(bOB + n);
            m_indices.push_back(bOT + j); m_indices.push_back(bOT + n);
            m_indices.push_back(bIB + j); m_indices.push_back(bIB + n);
            m_indices.push_back(bIT + j); m_indices.push_back(bIT + n);
            // Jumpers
            m_indices.push_back(bOB + j); m_indices.push_back(bIB + j);
            m_indices.push_back(bOT + j); m_indices.push_back(bIT + j);
        }
        mod.indexCount = (int)m_indices.size() - mod.indexStart;

        // Surface геометри€ (треугольники)
        int triVertStart = (int)m_triangleVertices.size();
        mod.triIndexStart = (int)m_triangleIndices.size();

        for (int s = 0; s < 4; s++) {
            float r = (s < 2) ? mod.outerRadius : mod.innerRadius;
            float y = (s % 2 == 0) ? yBottom : yTop;
            for (int j = 0; j < CIRCLE_SEGMENTS; j++) {
                float a = 2.0f * (float)M_PI * j / CIRCLE_SEGMENTS;
                m_triangleVertices.push_back(glm::vec3(r * cosf(a), y, r * sinf(a)));
            }
        }

        int ob = triVertStart, ot = ob + CIRCLE_SEGMENTS;
        int ib = ot + CIRCLE_SEGMENTS, it = ib + CIRCLE_SEGMENTS;

        for (int j = 0; j < CIRCLE_SEGMENTS; j++) {
            int n = (j + 1) % CIRCLE_SEGMENTS;
            m_triangleIndices.push_back(ob + j); m_triangleIndices.push_back(ob + n); m_triangleIndices.push_back(ot + j);
            m_triangleIndices.push_back(ob + n); m_triangleIndices.push_back(ot + n); m_triangleIndices.push_back(ot + j);
            m_triangleIndices.push_back(ib + j); m_triangleIndices.push_back(it + j); m_triangleIndices.push_back(ib + n);
            m_triangleIndices.push_back(ib + n); m_triangleIndices.push_back(it + j); m_triangleIndices.push_back(it + n);
            m_triangleIndices.push_back(ot + j); m_triangleIndices.push_back(ot + n); m_triangleIndices.push_back(it + j);
            m_triangleIndices.push_back(ot + n); m_triangleIndices.push_back(it + n); m_triangleIndices.push_back(it + j);
            m_triangleIndices.push_back(ob + j); m_triangleIndices.push_back(ib + j); m_triangleIndices.push_back(ob + n);
            m_triangleIndices.push_back(ob + n); m_triangleIndices.push_back(ib + j); m_triangleIndices.push_back(ib + n);
        }
        mod.triIndexCount = (int)m_triangleIndices.size() - mod.triIndexStart;

        currentY += mod.height + 0.05f;
    }
}