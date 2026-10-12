#ifndef VALIDATION_H
#define VALIDATION_H

#include <vector>
#include <string>
#include "AppState.h"
#include "CentralSolenoid.h"
#include "physics.h"

namespace Validation {

    void runAxialTest(const CentralSolenoid& solenoid, AppState& state);

    void runRadialTest(const CentralSolenoid& solenoid, AppState& state);

    void validateFieldToCSV(const CentralSolenoid& solenoid);

    void plotData(const std::vector<ValidationPoint>& data, const std::string& title, const std::string& xLabel);

} 

#endif // VALIDATION_H