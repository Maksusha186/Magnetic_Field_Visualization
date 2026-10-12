#ifndef CENTRAL_SOLENOID_H
#define CENTRAL_SOLENOID_H

#include <glm/glm.hpp>
#include <vector>

struct CSModuleInfo {
    float yCenter;
    float height;
    float innerRadius;
    float outerRadius;
    float current;
    int turns;

    // Control flags
    bool isVisible = true;            
    bool isCalculationEnabled = true; 
    bool showProjection = true;

    // Indices for lines
    int vertexStart;
    int vertexCount;
    int indexStart;
    int indexCount;

    // Let's add indexes to the triangles (to hide the rings individually)
    int triIndexStart;
    int triIndexCount;
};

class CentralSolenoid {
public:
    CentralSolenoid();
    void generateMesh();

    void resetToDefaults() {
        m_modules.clear();

        // 6 modules with initial parameters from class constants
        for (int i = 0; i < NUM_MODULES; i++) {
            CSModuleInfo mod;
            mod.height = MODULE_HEIGHT;
            mod.innerRadius = INNER_RADIUS;
            mod.outerRadius = OUTER_RADIUS;
            mod.current = 0.0f; 
            mod.turns = 500;    // Standard number of turns

            mod.yCenter = -TOTAL_HEIGHT / 2.0f + MODULE_HEIGHT / 2.0f + (i * MODULE_HEIGHT);

            mod.isVisible = true;
            mod.isCalculationEnabled = true;
            mod.showProjection = true;

            m_modules.push_back(mod);
        }

        generateMesh();         // Reassembling the geometry
        needsBufferUpdate = true; // We signal main.cpp to update video memory
    }

    const std::vector<glm::vec3>& getVertices() const { return m_vertices; }
    const std::vector<unsigned int>& getIndices() const { return m_indices; }
    const std::vector<glm::vec3>& getTriangleVertices() const { return m_triangleVertices; }
    const std::vector<unsigned int>& getTriangleIndices() const { return m_triangleIndices; }

    
    const std::vector<CSModuleInfo>& getModules() const { return m_modules; }
    std::vector<CSModuleInfo>& getModulesMutable() { return m_modules; }

    static constexpr float INNER_RADIUS = 1.3f;
    static constexpr float OUTER_RADIUS = 2.1f;
    static constexpr float MODULE_HEIGHT = 2.15f;
    static constexpr int NUM_MODULES = 6;
    static constexpr float TOTAL_HEIGHT = MODULE_HEIGHT * NUM_MODULES;
    static constexpr float CURRENT_PER_MODULE = 45000.0f;

    bool needsBufferUpdate = false;

private:
    std::vector<glm::vec3> m_vertices;
    std::vector<unsigned int> m_indices;
    std::vector<glm::vec3> m_triangleVertices;
    std::vector<unsigned int> m_triangleIndices;
    std::vector<CSModuleInfo> m_modules;
};

#endif