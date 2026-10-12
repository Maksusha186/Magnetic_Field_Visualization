#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include <iostream>
#include <vector>

#include "AppState.h"
#include "camera.h"
#include "CentralSolenoid.h"
#include "Shader.h"
#include "SliceRenderer.h"
#include "LineRenderer.h"
#include "Validation.h"
#include "ProjectManager.h"
#include "UIManager.h"

const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

AppState state;
Camera camera(glm::vec3(-15.0f, 0.0f, 0.0f));

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

unsigned int axesVAO = 0, axesVBO = 0;
unsigned int projVAO = 0, projVBO = 0;
std::vector<float> projVertices;

unsigned int solenoidLineVAO = 0, solenoidLineVBO = 0, solenoidLineEBO = 0;
unsigned int solenoidTriVAO = 0, solenoidTriVBO = 0, solenoidTriEBO = 0;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

void updateSolenoidProjection(const CentralSolenoid& solenoid);
void initAxesGeometry();
void initSolenoidBuffers(const CentralSolenoid& solenoid);
void updateSolenoidBuffers(const CentralSolenoid& solenoid);

int main() {

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "ITER CS Field Slice", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);


    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);


    Shader solidShader, sliceShader;
    solidShader.compileFromSource(ShaderSources::SOLID_VERTEX, ShaderSources::SOLID_FRAGMENT);
    sliceShader.compileFromSource(ShaderSources::SLICE_VERTEX, ShaderSources::SLICE_FRAGMENT);

    CentralSolenoid solenoid;
    SliceRenderer sliceRenderer;
    LineRenderer lineRenderer;

    sliceRenderer.init();
    lineRenderer.init();
    UIManager::init(window);


    initSolenoidBuffers(solenoid);
    initAxesGeometry();
    updateSolenoidProjection(solenoid);

    Validation::validateFieldToCSV(solenoid);
    sliceRenderer.startCalculation(state, solenoid, 0.0f, -12.0f, 12.0f, -12.0f, 12.0f);


    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();
        processInput(window);

        if (solenoid.needsBufferUpdate) {
            updateSolenoidBuffers(solenoid);
            updateSolenoidProjection(solenoid);
            solenoid.needsBufferUpdate = false;
        }

        sliceRenderer.checkAsyncResults(state);
        lineRenderer.checkAsyncResults(state);

        UIManager::newFrame();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 model = glm::mat4(1.0f);

        if (!state.showStartupWindow) {

            if (state.showSolenoid) {
                glEnable(GL_DEPTH_TEST);
                solidShader.use();
                solidShader.setMat4("view", view);
                solidShader.setMat4("projection", projection);
                solidShader.setMat4("model", model);

                const auto& mods = solenoid.getModules();

                glBindVertexArray(solenoidTriVAO);
                solidShader.setVec3("color", glm::vec3(0.2f, 0.8f, 0.2f));
                for (int i = 0; i < 6; i++) {
                    if (mods[i].isVisible)
                        glDrawElements(GL_TRIANGLES, mods[i].triIndexCount, GL_UNSIGNED_INT, (void*)(uintptr_t)(mods[i].triIndexStart * sizeof(unsigned int)));
                }

                glBindVertexArray(solenoidLineVAO);
                solidShader.setVec3("color", glm::vec3(0.0f, 0.0f, 0.0f));
                for (int i = 0; i < 6; i++) {
                    if (mods[i].isVisible)
                        glDrawElements(GL_LINES, mods[i].indexCount, GL_UNSIGNED_INT, (void*)(uintptr_t)(mods[i].indexStart * sizeof(unsigned int)));
                }
            }

            if (state.showField) {
                sliceRenderer.draw(sliceShader, view, projection, state);
            }

            if (state.showFieldLines) {
                lineRenderer.draw(solidShader, view, projection, state);
            }

            glDisable(GL_DEPTH_TEST);
            solidShader.use();
            solidShader.setMat4("view", view);
            solidShader.setMat4("projection", projection);
            solidShader.setMat4("model", model);

            if (state.showSolenoidProjection) {
                solidShader.setVec3("color", glm::vec3(0.0f, 0.0f, 0.0f));
                glLineWidth(2.0f);
                glBindVertexArray(projVAO);
                glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(projVertices.size() / 3));
            }

            if (state.showFieldAxes) {
                glLineWidth(2.0f);
                glBindVertexArray(axesVAO);

                solidShader.setVec3("color", glm::vec3(1.0f, 0.0f, 0.0f));
                glDrawArrays(GL_LINES, 0, 2);
                UIManager::draw3DLabel(glm::vec3(0.0f, 13.0f, 0.0f), "Z", view, projection, ImVec4(1, 0, 0, 1));

                solidShader.setVec3("color", glm::vec3(0.0f, 1.0f, 0.0f));
                glDrawArrays(GL_LINES, 2, 2);
                UIManager::draw3DLabel(glm::vec3(0.0f, 0.0f, 13.0f), "X", view, projection, ImVec4(0, 1, 0, 1));

                if (state.showXAxis) {
                    solidShader.setVec3("color", glm::vec3(0.0f, 0.0f, 1.0f));
                    glDrawArrays(GL_LINES, 4, 2);
                    UIManager::draw3DLabel(glm::vec3(-13.0f, 0.0f, 0.0f), "Y", view, projection, ImVec4(0, 0, 1, 1));
                }
            }
            glEnable(GL_DEPTH_TEST);
        }

        UIManager::renderUI(
            window, state, solenoid, camera, sliceRenderer, lineRenderer,
            [&]() { updateSolenoidProjection(solenoid); },
            view, projection
        );

        UIManager::renderDrawData();
        glfwSwapBuffers(window);
    }

    state.stopCalculation = true;
    state.stopLinesCalculation = true;

    if (state.calcThread.valid()) state.calcThread.wait();
    if (state.linesCalcThread.valid()) state.linesCalcThread.wait();

    UIManager::shutdown();
    glfwTerminate();
    return 0;
}

void initSolenoidBuffers(const CentralSolenoid& solenoid) {

    glGenVertexArrays(1, &solenoidLineVAO);
    glGenBuffers(1, &solenoidLineVBO);
    glGenBuffers(1, &solenoidLineEBO);

    glBindVertexArray(solenoidLineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, solenoidLineVBO);
    glBufferData(GL_ARRAY_BUFFER, solenoid.getVertices().size() * sizeof(glm::vec3), solenoid.getVertices().data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, solenoidLineEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, solenoid.getIndices().size() * sizeof(unsigned int), solenoid.getIndices().data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glGenVertexArrays(1, &solenoidTriVAO);
    glGenBuffers(1, &solenoidTriVBO);
    glGenBuffers(1, &solenoidTriEBO);

    glBindVertexArray(solenoidTriVAO);
    glBindBuffer(GL_ARRAY_BUFFER, solenoidTriVBO);
    glBufferData(GL_ARRAY_BUFFER, solenoid.getTriangleVertices().size() * sizeof(glm::vec3), solenoid.getTriangleVertices().data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, solenoidTriEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, solenoid.getTriangleIndices().size() * sizeof(unsigned int), solenoid.getTriangleIndices().data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void updateSolenoidBuffers(const CentralSolenoid& solenoid) {
    glBindBuffer(GL_ARRAY_BUFFER, solenoidLineVBO);
    glBufferData(GL_ARRAY_BUFFER, solenoid.getVertices().size() * sizeof(glm::vec3), solenoid.getVertices().data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, solenoidTriVBO);
    glBufferData(GL_ARRAY_BUFFER, solenoid.getTriangleVertices().size() * sizeof(glm::vec3), solenoid.getTriangleVertices().data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void initAxesGeometry() {
    float axesVertices[] = {
        0.0f, -12.0f, 0.0f,   0.0f,  12.0f, 0.0f,
        0.0f,  0.0f, -12.0f,  0.0f,  0.0f,  12.0f,
       -12.0f, 0.0f,  0.0f,  12.0f, 0.0f,  0.0f
    };

    glGenVertexArrays(1, &axesVAO);
    glGenBuffers(1, &axesVBO);
    glBindVertexArray(axesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axesVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axesVertices), axesVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void updateSolenoidProjection(const CentralSolenoid& solenoid) {
    projVertices.clear();
    const auto& mods = solenoid.getModules();

    for (const auto& m : mods) {
        if (!m.showProjection) continue;
        float yMin = m.yCenter - m.height / 2.0f;
        float yMax = m.yCenter + m.height / 2.0f;
        float sides[2] = { 1.0f, -1.0f };

        for (float s : sides) {
            float rIn = m.innerRadius * s;
            float rOut = m.outerRadius * s;

            float rect[] = {
                0.01f, yMin, rIn,  0.01f, yMax, rIn,
                0.01f, yMax, rIn,  0.01f, yMax, rOut,
                0.01f, yMax, rOut, 0.01f, yMin, rOut,
                0.01f, yMin, rOut, 0.01f, yMin, rIn
            };
            projVertices.insert(projVertices.end(), rect, rect + 24);
        }
    }

    if (projVAO == 0) glGenVertexArrays(1, &projVAO);
    if (projVBO == 0) glGenBuffers(1, &projVBO);

    glBindVertexArray(projVAO);
    glBindBuffer(GL_ARRAY_BUFFER, projVBO);
    glBufferData(GL_ARRAY_BUFFER, projVertices.size() * sizeof(float), projVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    if (state.orbitMode) {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS) {
            return;
        }
    }

    if (ImGui::GetIO().WantCaptureMouse) return;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}