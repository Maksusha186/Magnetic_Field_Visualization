#include "ProjectManager.h"
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

std::string ProjectManager::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &time);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buf);
}

void ProjectManager::deleteProject(const std::string& name) {
    try {
        fs::remove("projects/" + name + ".json");
        fs::remove("projects/" + name + ".field");
        fs::remove("projects/" + name + ".lines");
        fs::remove("projects/" + name + ".arrows");
    }
    catch (...) {}
}

std::vector<ProjectInfo> ProjectManager::listProjects() {
    std::vector<ProjectInfo> projects;
    if (!fs::exists("projects")) fs::create_directory("projects");

    for (const auto& entry : fs::directory_iterator("projects")) {
        if (entry.path().extension() == ".json") {
            try {
                std::ifstream i(entry.path());
                json j;
                i >> j;
                projects.push_back({
                    entry.path().stem().string(),
                    j.value("created", "Unknown"),
                    j.value("modified", "Unknown")
                    });
            }
            catch (...) {}
        }
    }
    return projects;
}

void ProjectManager::save(const std::string& name, const AppState& state, const CentralSolenoid& solenoid, const Camera& camera) {
    if (!fs::exists("projects")) fs::create_directory("projects");

    json j;

    std::string createdDate = getCurrentTimestamp();
    std::ifstream oldFile("projects/" + name + ".json");
    if (oldFile.is_open()) {
        json oldJ;
        oldFile >> oldJ;
        createdDate = oldJ.value("created", createdDate);
        oldFile.close();
    }

    j["name"] = name;
    j["created"] = createdDate;
    j["modified"] = getCurrentTimestamp();

    j["visuals"] = {
        {"showField", state.showField},
        {"showSolenoid", state.showSolenoid},
        {"showLines", state.showFieldLines},
        {"brightness", state.fieldBrightness},
        {"contrast", state.fieldContrast}
    };

    j["camera"] = {
        {"pos", {camera.Position.x, camera.Position.y, camera.Position.z}},
        {"yaw", camera.Yaw},
        {"pitch", camera.Pitch}
    };

    j["modules"] = json::array();
    for (const auto& m : solenoid.getModules()) {
        j["modules"].push_back({
            {"rIn", m.innerRadius}, {"rOut", m.outerRadius}, {"h", m.height},
            {"cur", m.current}, {"turns", m.turns},
            {"vis", m.isVisible}, {"phys", m.isCalculationEnabled}
            });
    }

    // Запись основного JSON файла
    std::ofstream o("projects/" + name + ".json");
    o << std::setw(4) << j << std::endl;
    o.close();

    // Сохранение бинарного файла среза (.field)
    if (state.textureValid && !state.fieldData.empty()) {
        std::ofstream bf("projects/" + name + ".field", std::ios::binary);
        bf.write((char*)state.fieldData.data(), state.fieldData.size() * sizeof(float));
        bf.close();
    }

    // Сохранение бинарного файла силовых линий (.lines)
    if (!state.lineVertices.empty()) {
        std::ofstream lf("projects/" + name + ".lines", std::ios::binary);
        lf.write((char*)state.lineVertices.data(), state.lineVertices.size() * sizeof(float));
        lf.close();
    }

    // Сохранение бинарного файла стрелок (.arrows)
    if (!state.arrowVertices.empty()) {
        std::ofstream af("projects/" + name + ".arrows", std::ios::binary);
        af.write((char*)state.arrowVertices.data(), state.arrowVertices.size() * sizeof(float));
        af.close();
    }
}

bool ProjectManager::load(const std::string& name, AppState& state, CentralSolenoid& solenoid, Camera& camera) {
    std::ifstream i("projects/" + name + ".json");
    if (!i.is_open()) return false;

    json j;
    i >> j;
    i.close();

    state.showField = j["visuals"]["showField"];
    state.showSolenoid = j["visuals"]["showSolenoid"];
    state.showFieldLines = j["visuals"]["showLines"];
    state.fieldBrightness = j["visuals"]["brightness"];
    state.fieldContrast = j["visuals"]["contrast"];

    camera.Position = glm::vec3(j["camera"]["pos"][0], j["camera"]["pos"][1], j["camera"]["pos"][2]);
    camera.Yaw = j["camera"]["yaw"];
    camera.Pitch = j["camera"]["pitch"];
    camera.updateCameraVectors();

    auto& mods = solenoid.getModulesMutable();
    for (int idx = 0; idx < 6; idx++) {
        if (idx >= mods.size()) break;
        auto& m = mods[idx];
        auto& mj = j["modules"][idx];
        m.innerRadius = mj["rIn"];
        m.outerRadius = mj["rOut"];
        m.height = mj["h"];
        m.current = mj["cur"];
        m.turns = mj["turns"];
        m.isVisible = mj["vis"];
        m.isCalculationEnabled = mj["phys"];
    }

    solenoid.generateMesh();
    solenoid.needsBufferUpdate = true;

    // Чтение файла среза (.field)
    std::ifstream f("projects/" + name + ".field", std::ios::binary);
    if (f) {
        state.fieldData.resize(state.textureWidth * state.textureHeight);
        f.read((char*)state.fieldData.data(), state.fieldData.size() * sizeof(float));
        f.close();
        state.textureValid = true;
    }
    else {
        state.textureValid = false;
    }

    // Чтение файла линий (.lines)
    std::ifstream lf("projects/" + name + ".lines", std::ios::binary);
    if (lf) {
        lf.seekg(0, std::ios::end);
        size_t size = lf.tellg();
        lf.seekg(0, std::ios::beg);
        state.lineVertices.resize(size / sizeof(float));
        lf.read((char*)state.lineVertices.data(), size);
        lf.close();
    }
    else {
        state.lineVertices.clear();
    }

    // Чтение файла стрелок (.arrows)
    std::ifstream af("projects/" + name + ".arrows", std::ios::binary);
    if (af) {
        af.seekg(0, std::ios::end);
        size_t size = af.tellg();
        af.seekg(0, std::ios::beg);
        state.arrowVertices.resize(size / sizeof(float));
        af.read((char*)state.arrowVertices.data(), size);
        af.close();
    }
    else {
        state.arrowVertices.clear();
    }

    state.projCreated = j.value("created", "-");
    state.projModified = j.value("modified", "-");
    state.currentProjectName = name;

    return true;
}