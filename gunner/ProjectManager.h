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

// Вспомогательная структура для списка проектов в меню (из оригинального main.cpp)
struct ProjectInfo {
    std::string name;
    std::string created;
    std::string modified;
};

class ProjectManager {
public:
    // Получение текущего времени в виде строки
    static std::string getCurrentTimestamp();

    // Удаление файлов проекта (.json, .field, .lines, .arrows)
    static void deleteProject(const std::string& name);

    // Получение списка всех проектов для окна Startup Window
    static std::vector<ProjectInfo> listProjects();

    // Сохранение полного состояния проекта
    static void save(const std::string& name, const AppState& state, const CentralSolenoid& solenoid, const Camera& camera);

    // Загрузка состояния проекта
    static bool load(const std::string& name, AppState& state, CentralSolenoid& solenoid, Camera& camera);
};

#endif // PROJECT_MANAGER_H