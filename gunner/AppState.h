#ifndef APP_STATE_H
#define APP_STATE_H

#include <string>
#include <vector>
#include <atomic>
#include <future>
#include <glm/glm.hpp>

struct ValidationPoint {
    float coord = 0.0f;
    float numericalB = 0.0f;
    float analyticalB = 0.0f;
};

struct AppState {
    AppState() = default;

    AppState(const AppState&) = delete;
    AppState& operator=(const AppState&) = delete;

    int windowMode = 1;
    bool showPanel = true;
    bool showStartupWindow = true;      
    bool showFilePanel = false;          
    bool orbitMode = true;               

    bool showSolenoid = true;            
    bool showField = true;               
    bool showFieldLines = true;          
    bool showFieldAxes = true;          
    bool showSolenoidProjection = true;  
    bool showXAxis = true;               
    int selectedModule = -1;             

    float fieldBrightness = 1.0f;        
    float fieldContrast = 1.0f;          

    bool showLineArrows = true;          
    int radialDensity = 5;               
    int angularDensity = 8;              
    int pointsPerLine = 2000;            
    float lineStepSize = 0.08f;          
    bool useAdaptiveStep = true;         

    bool showAxialTable = false;         
    bool showRadialTable = false;        
    std::vector<ValidationPoint> axialResults;
    std::vector<ValidationPoint> radialResults;

    std::string currentProjectName = "Untitled";
    char newProjNameBuf[64] = "MyProject";
    std::string projCreated = "-";
    std::string projModified = "-";

    std::atomic<float> calculationProgress{ 0.0f };
    std::atomic<bool> stopCalculation{ false };
    bool isCalculating = false;
    std::future<void> calcThread;

    int textureWidth = 512;
    int textureHeight = 512;
    std::vector<float> fieldData;
    bool textureValid = false;

    std::atomic<float> linesProgress{ 0.0f };
    std::atomic<bool> stopLinesCalculation{ false };
    bool isLinesCalculating = false;
    std::future<void> linesCalcThread;
    bool linesBufferNeedsUpdate = false;

    std::vector<float> lineVertices;         
    std::vector<float> arrowVertices;        
    std::vector<float> threadLineVertices;   
    std::vector<float> threadArrowVertices;  

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