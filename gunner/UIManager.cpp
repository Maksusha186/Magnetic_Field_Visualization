#include "UIManager.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "ProjectManager.h"
#include "Validation.h"
#include "physics.h"

#include <glm/gtc/matrix_transform.hpp>
#include <thread>
#include <cmath>
#include <cstdio>
#include <fstream>

void UIManager::init(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void UIManager::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UIManager::newFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UIManager::renderDrawData() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

ImVec4 UIManager::getFieldColor(float t) {
    if (t < 0.25f) return ImVec4(0.0f, 0.5f - t * 2.0f, 1.0f, 1.0f);
    if (t < 0.50f) return ImVec4(0.0f, 1.0f, 1.0f - (t - 0.25f) * 4.0f, 1.0f);
    if (t < 0.75f) return ImVec4((t - 0.50f) * 4.0f, 1.0f, 0.0f, 1.0f);
    return ImVec4(1.0f, 1.0f - (t - 0.75f) * 4.0f, 0.0f, 1.0f);
}

glm::vec3 UIManager::getMouseWorldCoords(GLFWwindow* window, glm::mat4 projection, glm::mat4 view) {
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    float depth;
    glReadPixels((int)mouseX, height - (int)mouseY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

    glm::vec4 viewport = glm::vec4(0, 0, width, height);
    glm::vec3 winPos = glm::vec3(mouseX, height - mouseY, depth);
    glm::vec3 worldPos = glm::unProject(winPos, view, projection, viewport);

    return worldPos;
}

void UIManager::draw3DLabel(glm::vec3 worldPos, const char* text, glm::mat4 view, glm::mat4 projection, ImVec4 color) {
    int width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
    glm::vec4 clipSpacePos = projection * view * glm::vec4(worldPos, 1.0f);

    if (clipSpacePos.w <= 0) return;

    glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;
    ImVec2 screenPos = ImVec2((ndcSpacePos.x + 1.0f) * 0.5f * width, (1.0f - ndcSpacePos.y) * 0.5f * height);

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    drawList->AddText(NULL, 30.0f, screenPos, ImColor(color), text);
}

void UIManager::setCameraPreset(Camera& camera, int mode) {
    float dist = 25.0f;
    switch (mode) {
    case 0:
        camera.Position = glm::vec3(-dist, 0.0f, 0.0f);
        camera.Yaw = 0.0f;
        camera.Pitch = 0.0f;
        break;

    case 1:
        camera.Position = glm::vec3(0.0f, 0.0f, dist);
        camera.Yaw = -90.0f;
        camera.Pitch = 0.0f;
        break;

    case 2:
        camera.Position = glm::vec3(0.0f, dist, 0.0f);
        camera.Yaw = 0.0f;
        camera.Pitch = -89.9f;
        break;
    }
    camera.ProcessMouseMovement(0.0f, 0.0f);
}

void UIManager::renderUI(
    GLFWwindow* window,
    AppState& state,
    CentralSolenoid& solenoid,
    Camera& camera,
    SliceRenderer& sliceRenderer,
    LineRenderer& lineRenderer,
    const std::function<void()>& updateProjectionCallback,
    const glm::mat4& view,
    const glm::mat4& projection)
{
    ImGuiIO& io = ImGui::GetIO();

    if (state.showStartupWindow) {
        renderStartupWindow(state, solenoid, camera, sliceRenderer, lineRenderer, updateProjectionCallback);
        return;
    }

    if (!state.showPanel) {
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 50, 20));
        ImGui::Begin("OpenPanel", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);

        if (ImGui::Button("<<", ImVec2(40, 40))) state.showPanel = true;

        ImGui::End();
    }

    if (state.showPanel) {
        renderControlPanel(window, state, solenoid, camera, sliceRenderer, lineRenderer, updateProjectionCallback);
    }

    renderValidationTables(state);
    renderInfoOverlay(window, state, solenoid, view, projection);
    renderProjectMenu(state, solenoid, camera);
}

void UIManager::renderStartupWindow(
    AppState& state,
    CentralSolenoid& solenoid,
    Camera& camera,
    SliceRenderer& sliceRenderer,
    LineRenderer& lineRenderer,
    const std::function<void()>& updateProjectionCallback)
{
    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(550, 450));
    ImGui::Begin("ITER Project Selector", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    ImGui::Text("CREATE NEW PROJECT");
    ImGui::InputText("Name##new", state.newProjNameBuf, 64);
    if (ImGui::Button("Create & Open", ImVec2(-1, 30))) {
        state.currentProjectName = state.newProjNameBuf;

        state.stopCalculation = true;
        state.stopLinesCalculation = true;
        solenoid.resetToDefaults();

        state.clearSliceData();
        state.clearLinesData();

        lineRenderer.uploadBufferData(state);

        if (updateProjectionCallback) updateProjectionCallback();
        ProjectManager::save(state.currentProjectName, state, solenoid, camera);
        state.showStartupWindow = false;
    }

    ImGui::Separator();
    ImGui::Text("LOAD RECENT PROJECT");

    if (ImGui::BeginChild("ProjectList", ImVec2(0, 0), true)) {
        auto projects = ProjectManager::listProjects();
        for (auto& p : projects) {

            ImGui::PushID(p.name.c_str());

            ImGui::BeginGroup();
            ImGui::BulletText("%s", p.name.c_str());
            ImGui::TextDisabled("Created: %s | Edited: %s", p.created.c_str(), p.modified.c_str());
            ImGui::EndGroup();

            ImGui::SameLine(ImGui::GetWindowWidth() - 150);
            if (ImGui::Button("Open", ImVec2(60, 0))) {
                // 1. Ожидаем завершения фоновых расчетов
                state.stopCalculation = true;
                state.stopLinesCalculation = true;
                if (state.calcThread.valid()) state.calcThread.wait();
                if (state.linesCalcThread.valid()) state.linesCalcThread.wait();

                state.isCalculating = false;
                state.isLinesCalculating = false;

                state.lineVertices.clear();
                state.arrowVertices.clear();
                state.textureValid = false;

                // 2. Загрузка файла с диска
                if (ProjectManager::load(p.name, state, solenoid, camera)) {
                    state.currentProjectName = p.name;

                    // 3. Отправка загруженных данных в GPU
                    if (state.textureValid) {
                        sliceRenderer.uploadTextureData(state);
                    }

                    lineRenderer.uploadBufferData(state);

                    if (updateProjectionCallback) updateProjectionCallback();
                    state.showStartupWindow = false;
                }
            }

            ImGui::SameLine(ImGui::GetWindowWidth() - 80);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("Del", ImVec2(60, 0))) {
                ProjectManager::deleteProject(p.name);
            }
            ImGui::PopStyleColor();

            ImGui::Separator();

            ImGui::PopID();
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void UIManager::renderControlPanel(
    GLFWwindow* window,
    AppState& state,
    CentralSolenoid& solenoid,
    Camera& camera,
    SliceRenderer& sliceRenderer,
    LineRenderer& lineRenderer,
    const std::function<void()>& updateProjectionCallback)
{
    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 320, 0));
    ImGui::SetNextWindowSize(ImVec2(320, io.DisplaySize.y));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("Control Panel", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

    if (ImGui::Button("Hide Panel >>", ImVec2(-1, 30))) state.showPanel = false;

    // SETTINGS (window)
    if (ImGui::CollapsingHeader("SETTINGS")) {
        ImGui::Indent();
        ImGui::Text("Window Mode:");
        if (ImGui::RadioButton("Fullscreen", &state.windowMode, 0)) {
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }
        if (ImGui::RadioButton("Windowed", &state.windowMode, 1)) {
            glfwSetWindowMonitor(window, NULL, 100, 100, 800, 600, 0);
        }
        if (ImGui::RadioButton("Borderless", &state.windowMode, 2)) {
            glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
        }
        ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("FIELD VISUALS")) {
        ImGui::Checkbox("Show Coordinate Axes (X/Y)", &state.showFieldAxes);
        ImGui::Checkbox("Show Solenoid Projection", &state.showSolenoidProjection);

        ImGui::SeparatorText("Post-Process");
        ImGui::SliderFloat("Brightness", &state.fieldBrightness, 0.1f, 2.0f);
        ImGui::SliderFloat("Contrast", &state.fieldContrast, 0.1f, 2.0f);

        if (ImGui::Button("Reset Defaults")) {
            state.fieldBrightness = 1.0f;
            state.fieldContrast = 1.0f;
        }
    }

    // RING SETTINGS
    if (ImGui::CollapsingHeader("RING SETTINGS")) {
        const char* names[] = { "CS3U", "CS2U", "CS1U", "CS1L", "CS2L", "CS3L" };
        auto& mods = solenoid.getModulesMutable();

        for (int i = 0; i < 6; i++) {
            int idx = 5 - i;

            bool isSelected = (state.selectedModule == i);
            if (isSelected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));

            if (ImGui::Button(names[i], ImVec2(-1, 35))) state.selectedModule = i;

            if (isSelected) {
                ImGui::PopStyleColor();
                ImGui::Indent();

                bool changed = false;

                if (ImGui::Checkbox("Visible", &mods[idx].isVisible)) changed = true;
                ImGui::SameLine();

                if (ImGui::Checkbox("Proj", &mods[idx].showProjection)) {
                    if (updateProjectionCallback) updateProjectionCallback();
                }
                ImGui::SameLine();

                if (ImGui::Checkbox("Physics", &mods[idx].isCalculationEnabled)) changed = true;

                auto DualInput = [&](const char* label, float* value, float min, float max, const char* format) {
                    ImGui::PushID(label);
                    ImGui::Text("%s", label);

                    ImGui::SetNextItemWidth(120);
                    if (ImGui::SliderFloat("##s", value, min, max, "")) changed = true;

                    ImGui::SameLine();

                    ImGui::SetNextItemWidth(80);
                    if (ImGui::InputFloat("##i", value, 0.0f, 0.0f, format)) changed = true;

                    ImGui::PopID();
                    };

                DualInput("Current (A)", &mods[idx].current, -60000.0f, 60000.0f, "%.0f");

                ImGui::Text("Turns count");
                ImGui::SetNextItemWidth(120);
                if (ImGui::SliderInt("##s_t", &mods[idx].turns, 1, 2000)) changed = true;
                ImGui::SameLine();
                ImGui::SetNextItemWidth(80);
                if (ImGui::InputInt("##i_t", &mods[idx].turns)) changed = true;

                ImGui::Separator();
                ImGui::Text("Geometry:");

                DualInput("Inner Radius (m)", &mods[idx].innerRadius, 0.1f, 3.0f, "%.3f");
                DualInput("Outer Radius (m)", &mods[idx].outerRadius, 0.2f, 5.0f, "%.3f");
                DualInput("Height (m)", &mods[idx].height, 0.1f, 5.0f, "%.3f");

                if (mods[idx].innerRadius >= mods[idx].outerRadius)
                    mods[idx].outerRadius = mods[idx].innerRadius + 0.01f;

                if (changed) {
                    solenoid.generateMesh();
                    solenoid.needsBufferUpdate = true;
                    if (updateProjectionCallback) updateProjectionCallback();
                }
                ImGui::Unindent();
            }
        }
    }

    // PHYSICS ENGINE
    ImGui::SeparatorText("PHYSICS ENGINE");
    if (!state.isCalculating) {
        if (ImGui::Button("RECALCULATE FIELD", ImVec2(-1, 45))) {
            sliceRenderer.startCalculation(state, solenoid, 0.0f, -12.0f, 12.0f, -12.0f, 12.0f);
        }
    }
    else {
        ImGui::Text("Calculating: %.1f%%", state.calculationProgress.load() * 100.0f);
        ImGui::ProgressBar(state.calculationProgress.load(), ImVec2(-1, 0));
        if (ImGui::Button("STOP", ImVec2(-1, 0))) state.stopCalculation = true;
    }

    if (ImGui::CollapsingHeader("FIELD LINES (RK4)")) {
        ImGui::SliderInt("Radial Layers", &state.radialDensity, 1, 20);
        ImGui::SliderInt("Angular Lines", &state.angularDensity, 1, 32);

        ImGui::Separator();

        ImGui::Checkbox("ADAPTIVE STEP (Auto)", &state.useAdaptiveStep);
        ImGui::Checkbox("Show Direction Arrows", &state.showLineArrows);

        if (!state.useAdaptiveStep) {
            ImGui::SliderFloat("Manual Step", &state.lineStepSize, 0.01f, 0.5f);
        }
        else {
            ImGui::Text("Base Step: %.3f (Auto-adjusting)", state.lineStepSize);
        }

        ImGui::SliderInt("Max Points", &state.pointsPerLine, 100, 10000);

        if (!state.isLinesCalculating) {
            if (ImGui::Button("GENERATE LINES", ImVec2(-1, 35))) {
                lineRenderer.startCalculation(state, solenoid);
            }
        }
        else {
            ImGui::ProgressBar(state.linesProgress.load(), ImVec2(-1, 0));
            if (ImGui::Button("STOP")) state.stopLinesCalculation = true;
        }
    }

    // VALIDATION TESTS
    if (ImGui::CollapsingHeader("VALIDATION TESTS")) {
        ImGui::Indent();

        // Test 1
        ImGui::Text("Axial Test (Z-axis):");
        if (ImGui::Button("Run##1")) Validation::runAxialTest(solenoid, state);
        ImGui::SameLine();
        if (ImGui::Button("Table##1")) state.showAxialTable = true;
        ImGui::SameLine();
        if (ImGui::Button("Plot##1")) {
            auto data = state.axialResults;
            std::thread([data]() {
                Validation::plotData(data, "Axial Magnetic Field", "Z coordinate (m)");
                }).detach();
        }
        ImGui::Separator();

        // Test 2
        ImGui::Text("Radial Test (X-axis):");
        if (ImGui::Button("Run##2")) Validation::runRadialTest(solenoid, state);
        ImGui::SameLine();
        if (ImGui::Button("Table##2")) state.showRadialTable = true;
        ImGui::SameLine();
        if (ImGui::Button("Plot##2")) {
            auto data = state.radialResults;
            std::thread([data]() {
                Validation::plotData(data, "Radial Magnetic Field", "X coordinate (m)");
                }).detach();
        }
        ImGui::Unindent();
    }

    // GLOBAL TOGGLES
    ImGui::SetCursorPosY(io.DisplaySize.y - 125);
    ImGui::Separator();

    ImGui::BeginGroup();
    ImGui::Text("VIEW PRESETS:");
    if (ImGui::Button("Front View (Y)", ImVec2(130, 25))) setCameraPreset(camera, 0);
    if (ImGui::Button("Side View (X)", ImVec2(130, 25))) setCameraPreset(camera, 1);
    if (ImGui::Button("Top View (Z)", ImVec2(130, 25))) setCameraPreset(camera, 2);
    ImGui::EndGroup();

    ImGui::SameLine(0, 15);

    ImGui::BeginGroup();
    ImGui::Checkbox("Show Field Slice", &state.showField);
    ImGui::Checkbox("Show Solenoid Model", &state.showSolenoid);
    ImGui::Checkbox("Show Magnetic Lines", &state.showFieldLines);
    ImGui::Checkbox("Show Depth Axis (Y)", &state.showXAxis);
    if (ImGui::Checkbox("Free Cursor (LMB)", &state.orbitMode)) {
        if (state.orbitMode) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    ImGui::EndGroup();

    ImGui::End();
    ImGui::PopStyleVar(2);
}

void UIManager::renderValidationTables(AppState& state) {
    if (state.showAxialTable) {
        ImGui::Begin("Axial Validation Table", &state.showAxialTable);
        if (ImGui::BeginTable("table1", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Y (m)");
            ImGui::TableSetupColumn("Num (T)");
            ImGui::TableSetupColumn("Ana (T)");
            ImGui::TableSetupColumn("Err (%)");
            ImGui::TableHeadersRow();

            for (auto& p : state.axialResults) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%.2f", p.coord);
                ImGui::TableSetColumnIndex(1); ImGui::Text("%.6f", p.numericalB);
                ImGui::TableSetColumnIndex(2); ImGui::Text("%.6f", p.analyticalB);
                float err = std::abs(p.numericalB - p.analyticalB) / (p.analyticalB + 1e-9f) * 100.0f;
                ImGui::TableSetColumnIndex(3); ImGui::Text("%.5f%%", err);
            }
            ImGui::EndTable();
        }
        ImGui::End();
    }

    if (state.showRadialTable) {
        ImGui::Begin("Radial Accuracy Table (X-axis)", &state.showRadialTable);
        if (ImGui::BeginTable("radial_table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("X (m)");
            ImGui::TableSetupColumn("Fast (T)");
            ImGui::TableSetupColumn("Reference (T)");
            ImGui::TableSetupColumn("Error (%)");
            ImGui::TableHeadersRow();

            for (auto& p : state.radialResults) {
                ImGui::TableNextRow();
                float error = std::abs(p.numericalB - p.analyticalB) / (p.analyticalB + 1e-9f) * 100.0f;

                ImGui::TableSetColumnIndex(0); ImGui::Text("%.2f", p.coord);
                ImGui::TableSetColumnIndex(1); ImGui::Text("%.6f", p.numericalB);
                ImGui::TableSetColumnIndex(2); ImGui::Text("%.6f", p.analyticalB);

                if (error > 1.0f) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.4f, 0.4f, 1));
                ImGui::TableSetColumnIndex(3); ImGui::Text("%.5f %%", error);
                if (error > 1.0f) ImGui::PopStyleColor();
            }
            ImGui::EndTable();
        }
        ImGui::End();
    }
}

void UIManager::renderInfoOverlay(
    GLFWwindow* window,
    AppState& state,
    const CentralSolenoid& solenoid,
    const glm::mat4& view,
    const glm::mat4& projection)
{
    glm::vec3 current_worldPos = getMouseWorldCoords(window, projection, view);

    glm::vec3 B(0.0f);
    for (const auto& mod : solenoid.getModules()) {
        if (!mod.isCalculationEnabled) continue;
        B += MagneticField::fieldFromCSModuleNumerical(
            current_worldPos, mod.yCenter, mod.height,
            mod.innerRadius, mod.outerRadius,
            mod.current * (float)mod.turns, 16, 32
        );
    }
    float current_B_mag = glm::length(B);

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::Begin("Info Overlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);

    // World coordinates
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "WORLD COORDINATES:");
    ImGui::Text("Y: %.3f  Z: %.3f  X: %.3f", -current_worldPos.x, current_worldPos.y, current_worldPos.z);

    ImGui::Separator();

    // System coordinates
    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "FIELD SYSTEM (2D):");
    ImGui::Text("Axial (Z):  %.3f m", current_worldPos.y);
    ImGui::Text("Radial (X): %.3f m", current_worldPos.z);

    ImGui::Separator();

    // Field value and scale
    ImGui::TextColored(ImVec4(0, 1, 1, 1), "FIELD B: %.6f T", current_B_mag);
    ImGui::Spacing();

    ImGui::Text("Scale (0 - 13T):");
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 start_p = ImGui::GetCursorScreenPos();
    float bar_w = 20.0f;
    float bar_h = 180.0f;

    for (int i = 0; i < (int)bar_h; ++i) {
        float t = (float)i / bar_h;
        ImVec4 col = getFieldColor(1.0f - t);
        draw_list->AddLine(ImVec2(start_p.x, start_p.y + i), ImVec2(start_p.x + bar_w, start_p.y + i), ImGui::ColorConvertFloat4ToU32(col));
    }

    for (int i = 0; i <= 4; ++i) {
        float val = 13.0f * (1.0f - (float)i / 4.0f);
        char buf[16];
        sprintf_s(buf, "%.1f T", val);
        draw_list->AddText(ImVec2(start_p.x + bar_w + 8, start_p.y + i * (bar_h / 4.0f) - 7), 0xFFFFFFFF, buf);
    }

    ImGui::Dummy(ImVec2(bar_w + 65, bar_h + 10));

    ImGui::End();
}

void UIManager::renderProjectMenu(
    AppState& state,
    const CentralSolenoid& solenoid,
    const Camera& camera)
{
    ImGuiIO& io = ImGui::GetIO();

    float filePanelHeight = 220.0f;
    float filePanelWidth = 250.0f;

    ImGui::SetNextWindowPos(ImVec2(10, io.DisplaySize.y - 40));
    ImGui::Begin("ProjectBtn", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);
    if (ImGui::Button(state.showFilePanel ? " CLOSE MENU " : " PROJECT MENU ", ImVec2(150, 30))) {
        state.showFilePanel = !state.showFilePanel;
    }
    ImGui::End();

    if (state.showFilePanel) {
        ImGui::SetNextWindowPos(ImVec2(10, io.DisplaySize.y - 45 - filePanelHeight));
        ImGui::SetNextWindowSize(ImVec2(filePanelWidth, filePanelHeight));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::Begin("Project Control Panel", &state.showFilePanel,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::TextColored(ImVec4(1, 1, 0, 1), "PROJECT: %s", state.currentProjectName.c_str());
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("SAVE", ImVec2(-1, 40))) {
            ProjectManager::save(state.currentProjectName, state, solenoid, camera);
            state.projModified = ProjectManager::getCurrentTimestamp();
        }

        ImGui::Spacing();

        if (ImGui::Button("NEW / LOAD OTHER", ImVec2(-1, 40))) {
            state.showStartupWindow = true;
            state.showFilePanel = false;
        }

        ImGui::Spacing();
        ImGui::SetCursorPosY(filePanelHeight - 60);
        ImGui::Separator();
        ImGui::TextDisabled("Created: %s", state.projCreated.c_str());
        ImGui::TextDisabled("Modified: %s", state.projModified.c_str());

        ImGui::End();
        ImGui::PopStyleVar();
    }

    if (ImGui::BeginPopup("FileMenuPopup")) {
        ImGui::Text("Current: %s", state.currentProjectName.c_str());
        ImGui::Separator();

        if (ImGui::MenuItem("Save Project")) {
            ProjectManager::save(state.currentProjectName, state, solenoid, camera);
        }

        if (ImGui::MenuItem("New Project / Load Other")) {
            state.showStartupWindow = true;
        }

        ImGui::Separator();
        ImGui::TextDisabled("Created: %s", state.projCreated.c_str());
        ImGui::TextDisabled("Last Edit: %s", state.projModified.c_str());

        ImGui::EndPopup();
    }
}