#ifndef PROJECT_MANAGER_H
#define PROJECT_MANAGER_H

#include <string>
#include <vector>
#include <filesystem>
#include "json.hpp"

#include "AppState.h"
#include "CentralSolenoid.h"
#include "camera.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

struct ProjectInfo {
    std::string name;
    std::string created;
    std::string modified;
};

class ProjectManager {
public:

    static std::string getCurrentTimestamp();

    static void deleteProject(const std::string& name);

    static std::vector<ProjectInfo> listProjects();

    static void save(const std::string& name, const AppState& state, const CentralSolenoid& solenoid, const Camera& camera);

    static bool load(const std::string& name, AppState& state, CentralSolenoid& solenoid, Camera& camera);
};

#endif // PROJECT_MANAGER_H