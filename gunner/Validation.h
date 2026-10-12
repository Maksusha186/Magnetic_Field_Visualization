#ifndef VALIDATION_H
#define VALIDATION_H

#include <vector>
#include <string>
#include "AppState.h"
#include "CentralSolenoid.h"
#include "physics.h"

namespace Validation {

    // Выполнение аксиального теста (вдоль центральной оси соленоида)
    void runAxialTest(const CentralSolenoid& solenoid, AppState& state);

    // Выполнение радиального теста (от центра наружу по радиусу)
    void runRadialTest(const CentralSolenoid& solenoid, AppState& state);

    // Экспорт результатов аксиального анализа в файл CSV
    void validateFieldToCSV(const CentralSolenoid& solenoid);

    // Отрисовка 2D-графика результатов через Matplot++
    void plotData(const std::vector<ValidationPoint>& data, const std::string& title, const std::string& xLabel);

} // namespace Validation

#endif // VALIDATION_H