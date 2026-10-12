#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <functional>

#include "imgui.h"

#include "AppState.h"
#include "CentralSolenoid.h"
#include "camera.h"
#include "SliceRenderer.h"
#include "LineRenderer.h"

class UIManager {
public:
    static void init(GLFWwindow* window);
    static void shutdown();
    static void newFrame();
    static void renderDrawData();

    static void draw3DLabel(glm::vec3 worldPos, const char* text, glm::mat4 view, glm::mat4 projection, ImVec4 color);
    static void setCameraPreset(Camera& camera, int mode);

    static void renderUI(
        GLFWwindow* window,
        AppState& state,
        CentralSolenoid& solenoid,
        Camera& camera,
        SliceRenderer& sliceRenderer,
        LineRenderer& lineRenderer,
        const std::function<void()>& updateProjectionCallback,
        const glm::mat4& view,
        const glm::mat4& projection
    );

private:

    static void renderStartupWindow(
        AppState& state,
        CentralSolenoid& solenoid,
        Camera& camera,
        SliceRenderer& sliceRenderer,
        LineRenderer& lineRenderer,
        const std::function<void()>& updateProjectionCallback
    );

    static void renderControlPanel(
        GLFWwindow* window,
        AppState& state,
        CentralSolenoid& solenoid,
        Camera& camera,
        SliceRenderer& sliceRenderer,
        LineRenderer& lineRenderer,
        const std::function<void()>& updateProjectionCallback
    );

    static void renderValidationTables(AppState& state);

    static void renderInfoOverlay(
        GLFWwindow* window,
        AppState& state,
        const CentralSolenoid& solenoid,
        const glm::mat4& view,
        const glm::mat4& projection
    );

    static void renderProjectMenu(
        AppState& state,
        const CentralSolenoid& solenoid,
        const Camera& camera
    );

    static glm::vec3 getMouseWorldCoords(GLFWwindow* window, glm::mat4 projection, glm::mat4 view);
    static ImVec4 getFieldColor(float t);
};

#endif // UI_MANAGER_H