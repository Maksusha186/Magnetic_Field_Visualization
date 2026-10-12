#include "Validation.h"
#include <matplot/matplot.h>

#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <thread>

namespace Validation {

    void runAxialTest(const CentralSolenoid& solenoid, AppState& state) {
        state.axialResults.clear();
        const float mu0 = 4.0f * 3.14159265f * 1e-7f;
        const auto& mods = solenoid.getModules();

        int nRad = 32;
        int nVert = 32;

        std::cout << "Running Axial Field Validation Test..." << std::endl;

        for (float y = -12.0f; y <= 12.0f; y += 0.2f) {
            glm::vec3 p(0.0f, y, 0.0f);

            glm::vec3 B_num_total_vec(0.0f);
            float B_ana_total = 0.0f;

            for (const auto& mod : mods) {
                if (!mod.isCalculationEnabled) continue;

                B_num_total_vec += MagneticField::fieldFromCSModuleNumerical(
                    p, mod.yCenter, mod.height, mod.innerRadius, mod.outerRadius,
                    mod.current * (float)mod.turns, nRad, nVert
                );

                float R1 = mod.innerRadius;
                float R2 = mod.outerRadius;
                float NI = mod.current * (float)mod.turns;
                float J_mod = NI / (mod.height * (R2 - R1));

                auto thick_solenoid_term = [R1, R2](float h) {
                    if (std::abs(h) < 1e-6f) return 0.0f;
                    return h * logf((R2 + sqrtf(R2 * R2 + h * h)) / (R1 + sqrtf(R1 * R1 + h * h)));
                    };

                float h1 = (y - mod.yCenter) + mod.height / 2.0f;
                float h2 = (y - mod.yCenter) - mod.height / 2.0f;

                B_ana_total += (mu0 * J_mod / 2.0f) * (thick_solenoid_term(h1) - thick_solenoid_term(h2));
            }

            float B_num = glm::length(B_num_total_vec);
            state.axialResults.push_back({ y, B_num, B_ana_total });
        }

        std::cout << "Axial Test Completed. Samples: " << state.axialResults.size() << std::endl;
    }

    void runRadialTest(const CentralSolenoid& solenoid, AppState& state) {
        state.radialResults.clear();
        const auto& mods = solenoid.getModules();

        std::cout << "Starting High-Precision Radial Test..." << std::endl;

        for (float x = 0.0f; x <= 10.0f; x += 0.2f) {
            glm::vec3 p(x, 0.0f, 0.0f);

            float B_fast = 0.0f; 
            float B_ref = 0.0f;  

            for (const auto& mod : mods) {
                if (!mod.isCalculationEnabled) continue;

                B_fast += glm::length(MagneticField::fieldFromCSModuleNumerical(
                    p, mod.yCenter, mod.height, mod.innerRadius, mod.outerRadius,
                    mod.current * (float)mod.turns, 32, 32
                ));

                B_ref += glm::length(MagneticField::fieldFromCSModuleNumerical(
                    p, mod.yCenter, mod.height, mod.innerRadius, mod.outerRadius,
                    mod.current * (float)mod.turns, 100, 100
                ));
            }

            state.radialResults.push_back({ x, B_fast, B_ref });
        }

        std::cout << "Radial Test Completed. Samples: " << state.radialResults.size() << std::endl;
    }

    void validateFieldToCSV(const CentralSolenoid& solenoid) {
        std::ofstream file("validation_results.csv");
        file << "Y_Coordinate;Numerical_B_Tesla;Analytical_B_Tesla;Error_Percent\n";

        const float mu0 = 4.0f * 3.14159265f * 1e-7f;
        const auto& mods = solenoid.getModules();

        float totalHeight = 0.0f;
        for (const auto& m : mods) totalHeight += m.height;
        float R1 = mods[0].innerRadius;
        float R2 = mods[0].outerRadius;

        float totalNI = 0.0f;
        for (const auto& m : mods) totalNI += m.current * (float)m.turns;

        float J = totalNI / (totalHeight * (R2 - R1));

        for (float y = -12.0f; y <= 12.0f; y += 0.2f) {
            glm::vec3 p(0.0f, y, 0.0f);

            glm::vec3 B_num_vec(0.0f);
            for (const auto& mod : mods) {
                if (!mod.isCalculationEnabled) continue;
                B_num_vec += MagneticField::fieldFromCSModuleNumerical(
                    p, mod.yCenter, mod.height, mod.innerRadius, mod.outerRadius,
                    mod.current * (float)mod.turns, 16, 32
                );
            }
            float B_num = glm::length(B_num_vec);

            auto thick_solenoid_term = [R1, R2](float h) {
                if (std::abs(h) < 1e-6f) return 0.0f;
                return h * logf((R2 + sqrtf(R2 * R2 + h * h)) / (R1 + sqrtf(R1 * R1 + h * h)));
                };

            float h1 = y + totalHeight / 2.0f;
            float h2 = y - totalHeight / 2.0f;
            float B_ana = (mu0 * J / 2.0f) * (thick_solenoid_term(h1) - thick_solenoid_term(h2));

            float error = std::abs(B_num - B_ana) / (B_ana + 1e-9f) * 100.0f;

            file << std::fixed << std::setprecision(6)
                << y << ";" << B_num << ";" << B_ana << ";" << error << "%\n";
        }

        file.close();
        std::cout << "Validation results saved to validation_results.csv" << std::endl;
    }

    void plotData(const std::vector<ValidationPoint>& data, const std::string& title, const std::string& xLabel) {
        if (data.empty()) return;

        std::vector<double> x, y_fast, y_ref;
        bool has_reference = false;

        for (const auto& p : data) {
            x.push_back(static_cast<double>(p.coord));
            y_fast.push_back(static_cast<double>(p.numericalB));
            y_ref.push_back(static_cast<double>(p.analyticalB));

            if (std::abs(p.analyticalB) > 1e-12f) {
                has_reference = true;
            }
        }

        // Создание нового графического окна через Matplot++
        auto f = matplot::figure(true);
        f->size(800, 600);

        auto p1 = matplot::plot(x, y_fast, "-o");
        p1->line_width(2);
        p1->marker_size(4);
        p1->display_name("Numerical (Fast)");

        if (has_reference) {
            matplot::hold(matplot::on);
            auto p2 = matplot::plot(x, y_ref, "r--");
            p2->line_width(2);
            p2->display_name("Reference / Analytical");
        }

        matplot::title(title);
        matplot::xlabel(xLabel);
        matplot::ylabel("Magnetic Field B (Tesla)");
        matplot::grid(matplot::on);
        matplot::legend()->location(matplot::legend::general_alignment::topright);

        matplot::show();
    }

} // namespace Validation