#ifndef PHYSICS_H
#define PHYSICS_H

#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <algorithm>
#include <functional>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

class MagneticField {
public:
    static constexpr float MU0 = 4.0f * M_PI * 1e-7f;
    static constexpr float MU0_4PI = 1e-7f;

    // Realization of Elliptic Integrals (Hastings Approximation)
    static void ellint_KE(float k, float& K, float& E) {
        float m = k * k;
        float m1 = 1.0f - m;

        // Coefficients for K(m)
        float a[] = { 1.38629436112f, 0.09666344259f, 0.03590092383f, 0.03742563713f, 0.01451196212f };
        float b[] = { 0.5f, 0.12498593597f, 0.06880248576f, 0.03328355346f, 0.00441787012f };
        // Coefficients for E(m)
        float c[] = { 1.0f, 0.44325141463f, 0.06260609976f, 0.04757383546f, 0.01736506451f };
        float d[] = { 0.0f, 0.24998368310f, 0.09200180037f, 0.04069697526f, 0.00526449639f };

        float lnm1 = logf(m1);
        K = (a[0] + m1 * (a[1] + m1 * (a[2] + m1 * (a[3] + m1 * a[4])))) -
            (b[0] + m1 * (b[1] + m1 * (b[2] + m1 * (b[3] + m1 * b[4])))) * lnm1;
        E = (c[0] + m1 * (c[1] + m1 * (c[2] + m1 * (c[3] + m1 * c[4])))) -
            (d[0] + m1 * (d[1] + m1 * (d[2] + m1 * (d[3] + m1 * d[4])))) * lnm1;
    }

    static inline glm::vec3 fieldFromThinLoop(const glm::vec3& p, float a, float y_center, float I) {
        float rho = sqrtf(p.x * p.x + p.z * p.z);
        float z = p.y - y_center;

        const float eps = 1e-6f;

        if (rho < eps) {
            // Аналитическое доопределение строго на оси
            float B_axial = (MU0 * I * a * a) / (2.0f * powf(a * a + z * z, 1.5f));
            return glm::vec3(0.0f, B_axial, 0.0f);
        }

        float r_max_sq = powf(a + rho, 2) + z * z;
        float r_min_sq = powf(a - rho, 2) + z * z;

        if (r_min_sq < 1e-12f) {
            r_min_sq = 1e-12f;
        }

        float k = sqrtf(4.0f * a * rho / r_max_sq);

        float K, E;
        ellint_KE(k, K, E);

        float common = (MU0 * I) / (2.0f * M_PI * sqrtf(r_max_sq));
        float By = common * (K + E * (a * a - rho * rho - z * z) / r_min_sq);
        float Br = common * (z / rho) * (-K + E * (a * a + rho * rho + z * z) / r_min_sq);

        return glm::vec3(Br * (p.x / rho), By, Br * (p.z / rho));
    }

    static glm::vec3 fieldFromCSModuleNumerical(const glm::vec3& p, float yCenter, float height,
        float innerRadius, float outerRadius, float current,
        int nRadial, int nVertical) {

        glm::vec3 B(0.0f);
        float dy = height / nVertical;
        float dr = (outerRadius - innerRadius) / nRadial;

        float I_per_point = current / (float)(nRadial * nVertical * 4);

        const float g = 0.577350269f;
        const float nodes[2] = { -g, g };

        for (int i = 0; i < nVertical; ++i) {
            float y_cell_center = (yCenter - height / 2.0f) + (i + 0.5f) * dy;

            for (int j = 0; j < nRadial; ++j) {
                float r_cell_center = innerRadius + (j + 0.5f) * dr;

                for (int g_y = 0; g_y < 2; ++g_y) {
                    float y = y_cell_center + nodes[g_y] * (dy * 0.5f);

                    for (int g_r = 0; g_r < 2; ++g_r) {
                        float r = r_cell_center + nodes[g_r] * (dr * 0.5f);

                        B += fieldFromThinLoop(p, r, y, I_per_point);
                    }
                }
            }
        }
        return B;
    }

    static glm::vec3 getNextLinePoint(glm::vec3 p, float stepSize, const std::function<glm::vec3(glm::vec3)>& fieldFunc) {
        auto k1 = fieldFunc(p);
        if (glm::length(k1) < 1e-9f) return p;
        k1 = glm::normalize(k1) * stepSize;

        auto k2 = fieldFunc(p + k1 * 0.5f);
        k2 = glm::normalize(k2) * stepSize;

        auto k3 = fieldFunc(p + k2 * 0.5f);
        k3 = glm::normalize(k3) * stepSize;

        auto k4 = fieldFunc(p + k3);
        k4 = glm::normalize(k4) * stepSize;

        return p + (k1 + 2.0f * k2 + 2.0f * k3 + k4) / 6.0f;
    }
};

#endif