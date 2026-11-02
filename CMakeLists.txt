cmake_minimum_required(VERSION 3.16)
project(CarSimulator LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------
# Външни библиотеки - изтеглят се автоматично при първото конфигуриране
# ------------------------------------------------------------------
include(FetchContent)

# GLFW - прозорец, OpenGL контекст, клавиатура и мишка
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE)

# GLM - математика: вектори, матрици, трансформации
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE)

FetchContent_MakeAvailable(glfw glm)

# GLAD - зареждане на OpenGL 3.3 Core функциите (генериран, в external/glad)
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# ------------------------------------------------------------------
# Приложението
# ------------------------------------------------------------------
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(CarSimulator ${SOURCES})
# stb_image - зареждане на PNG/JPG (единичен header файл)
target_include_directories(CarSimulator PRIVATE src external/stb)
target_link_libraries(CarSimulator PRIVATE glad glfw glm::glm)

# Пътят до папката с шейдъри и текстури - така програмата ги намира
# независимо от коя директория е стартирана
target_compile_definitions(CarSimulator PRIVATE
    ASSET_DIR="${CMAKE_SOURCE_DIR}/assets/")

if(MSVC)
    target_compile_options(CarSimulator PRIVATE /W3 /utf-8)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT CarSimulator)
else()
    target_compile_options(CarSimulator PRIVATE -Wall)
endif()
