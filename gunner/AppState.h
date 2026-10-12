#ifndef APP_STATE_H
#define APP_STATE_H

#include <string>
#include <vector>
#include <atomic>
#include <future>
#include <glm/glm.hpp>

// ---------------------------------------------------------------------
// Структура для результатов валидационных тестов (Аксиальный / Радиальный)
// ---------------------------------------------------------------------
struct ValidationPoint {
    float coord = 0.0f;
    float numericalB = 0.0f;
    float analyticalB = 0.0f;
};

// ---------------------------------------------------------------------
// Единое состояние приложения (AppState)
// ---------------------------------------------------------------------
struct AppState {
    AppState() = default;

    // Запрещаем копирование, так как внутри есть std::atomic и std::future
    AppState(const AppState&) = delete;
    AppState& operator=(const AppState&) = delete;

    // -----------------------------------------------------------------
    // 1. Окно и Главные Режимы Интерфейса
    // -----------------------------------------------------------------
    int windowMode = 1;                 // 0: Fullscreen, 1: Windowed, 2: Borderless
    bool showPanel = true;              // Отображение боковой панели управления
    bool showStartupWindow = true;       // Окно выбора/создания проекта при старте
    bool showFilePanel = false;          // Малое меню управления файлом проекта
    bool orbitMode = true;               // true: UI/свободный курсор, false: FPS-камера

    // -----------------------------------------------------------------
    // 2. Видимость Элементов Сцены
    // -----------------------------------------------------------------
    bool showSolenoid = true;            // Видимость 3D-модели соленоида
    bool showField = true;               // Видимость 2D-среза магнитного поля
    bool showFieldLines = true;          // Видимость 3D силовых линий (RK4)
    bool showFieldAxes = true;          // Видимость координатных осей (X/Y/Z)
    bool showSolenoidProjection = true;  // Контуры соленоида на 2D-срезе
    bool showXAxis = true;               // Видимость оси глубин
    int selectedModule = -1;             // Выбранный модуль соленоида (-1: ни один)

    // -----------------------------------------------------------------
    // 3. Пост-Обработка и Визуальные Эффекты
    // -----------------------------------------------------------------
    float fieldBrightness = 1.0f;        // Яркость текстуры среза
    float fieldContrast = 1.0f;          // Контраст текстуры среза

    // -----------------------------------------------------------------
    // 4. Параметры Трассировки Силовых Линий (Метод Рунге-Кутты 4)
    // -----------------------------------------------------------------
    bool showLineArrows = true;          // Стрелки направления поля на линиях
    int radialDensity = 5;               // Плотность начальных точек по радиусу
    int angularDensity = 8;              // Плотность начальных точек по углу
    int pointsPerLine = 2000;            // Максимальное кол-во сегментов линии
    float lineStepSize = 0.08f;          // Базовый шаг интегрирования
    bool useAdaptiveStep = true;         // Адаптивный выбор шага по кривизне

    // -----------------------------------------------------------------
    // 5. Данные Валидационных Тестов
    // -----------------------------------------------------------------
    bool showAxialTable = false;         // Окно таблицы аксиального теста
    bool showRadialTable = false;        // Окно таблицы радиального теста
    std::vector<ValidationPoint> axialResults;
    std::vector<ValidationPoint> radialResults;

    // -----------------------------------------------------------------
    // 6. Метаданные Проекта
    // -----------------------------------------------------------------
    std::string currentProjectName = "Untitled";
    char newProjNameBuf[64] = "MyProject"; // Чистая инициализация без strncpy
    std::string projCreated = "-";
    std::string projModified = "-";

    // -----------------------------------------------------------------
    // 7. Многопоточность: Расчет 2D-Среза Поля
    // -----------------------------------------------------------------
    std::atomic<float> calculationProgress{ 0.0f };
    std::atomic<bool> stopCalculation{ false };
    bool isCalculating = false;
    std::future<void> calcThread;

    // Буферы данных среза
    int textureWidth = 512;
    int textureHeight = 512;
    std::vector<float> fieldData;
    bool textureValid = false;

    // -----------------------------------------------------------------
    // 8. Многопоточность: Расчет Силовых Линий (RK4)
    // -----------------------------------------------------------------
    std::atomic<float> linesProgress{ 0.0f };
    std::atomic<bool> stopLinesCalculation{ false };
    bool isLinesCalculating = false;
    std::future<void> linesCalcThread;
    bool linesBufferNeedsUpdate = false;

    // Буферы вершин линий и стрелок
    std::vector<float> lineVertices;         // Буфер для рендеринга линий
    std::vector<float> arrowVertices;        // Буфер для рендеринга стрелок
    std::vector<float> threadLineVertices;   // Временный буфер потока линий
    std::vector<float> threadArrowVertices;  // Временный буфер потока стрелок

    // -----------------------------------------------------------------
    // Вспомогательные методы сброса состояния
    // -----------------------------------------------------------------
    void clearLinesData() {
        lineVertices.clear();
        arrowVertices.clear();
        threadLineVertices.clear();
        threadArrowVertices.clear();
        linesBufferNeedsUpdate = true;
    }

    void clearSliceData() {
        fieldData.assign(textureWidth * textureHeight, 0.0f);
        textureValid = false;
    }
};

#endif // APP_STATE_H