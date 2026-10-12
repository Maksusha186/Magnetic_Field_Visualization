cmake_minimum_required(VERSION 3.15)
project(gunner LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Библиотеки внутри проекта
set(LIBS_DIR "${CMAKE_SOURCE_DIR}/libraries")
set(GLFW_INCLUDE_DIR "${LIBS_DIR}/include")
set(GLFW_LIBRARY "${LIBS_DIR}/lib/glfw3.lib")

find_package(OpenGL REQUIRED)

add_executable(gunner
    main.cpp
    CentralSolenoid.cpp
    LineRenderer.cpp
    ProjectManager.cpp
    Shader.cpp
    SliceRenderer.cpp
    UIManager.cpp
    Validation.cpp
    glad.c
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_impl_glfw.cpp
    imgui/imgui_impl_opengl3.cpp
)

target_include_directories(gunner PRIVATE
    "."
    "imgui"
    ${GLFW_INCLUDE_DIR}
)

target_link_libraries(gunner PRIVATE
    ${GLFW_LIBRARY}
    OpenGL::GL
)