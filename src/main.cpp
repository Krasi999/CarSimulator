// ============================================================
//  Интерактивен 3D автомобилен симулатор
//  Етап 2: Терен (height map) и път (Catmull-Rom сплайн) с текстури
//
//  Управление:
//    W/A/S/D      - напред / наляво / назад / надясно
//    Space / Ctrl - нагоре / надолу
//    Shift        - по-бързо движение
//    Мишка        - оглеждане
//    Колелце      - приближаване (промяна на FOV)
//    F1           - каркасен режим (wireframe) вкл./изкл.
//    Tab          - освобождаване / прихващане на курсора
//    Esc          - изход
// ============================================================
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include "Texture.h"
#include "Road.h"
#include "Terrain.h"

// ------------------------------------------------------------
//  Глобално състояние на приложението (достъпно от callback-ите)
// ------------------------------------------------------------
struct AppState {
    int    width = 1280, height = 720;
    Camera camera;
    bool   firstMouse    = true;   // за да няма "скок" при първото движение
    double lastX = 0.0, lastY = 0.0;
    bool   mouseCaptured = true;
    bool   wireframe     = false;
};
static AppState g_app;

// Режими на оцветяване - съвпадат с uMode във basic.frag
enum ShadeMode { MODE_COLOR = 0, MODE_TEXTURE = 1, MODE_TERRAIN = 2 };

// ------------------------------------------------------------
//  Callback функции - GLFW ги извиква при събития
// ------------------------------------------------------------
static void framebufferSizeCallback(GLFWwindow*, int w, int h) {
    g_app.width = w;
    g_app.height = h;
    glViewport(0, 0, w, h);
}

static void cursorPosCallback(GLFWwindow*, double x, double y) {
    if (!g_app.mouseCaptured) return;

    if (g_app.firstMouse) {
        g_app.lastX = x;
        g_app.lastY = y;
        g_app.firstMouse = false;
    }
    float xOffset = static_cast<float>(x - g_app.lastX);
    float yOffset = static_cast<float>(g_app.lastY - y); // обърнато: Y на екрана расте надолу
    g_app.lastX = x;
    g_app.lastY = y;

    g_app.camera.processMouseMovement(xOffset, yOffset);
}

static void scrollCallback(GLFWwindow*, double, double yOffset) {
    g_app.camera.processMouseScroll(static_cast<float>(yOffset));
}

static void keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, true);

    if (key == GLFW_KEY_TAB) {
        g_app.mouseCaptured = !g_app.mouseCaptured;
        glfwSetInputMode(window, GLFW_CURSOR,
                         g_app.mouseCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        g_app.firstMouse = true;
    }

    // Каркасен режим - показва триъгълниците, от които е съставена сцената
    if (key == GLFW_KEY_F1) {
        g_app.wireframe = !g_app.wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, g_app.wireframe ? GL_LINE : GL_FILL);
    }
}

static void processInput(GLFWwindow* window, float dt) {
    auto down = [&](int key) { return glfwGetKey(window, key) == GLFW_PRESS; };
    bool fast = down(GLFW_KEY_LEFT_SHIFT);
    Camera& cam = g_app.camera;

    if (down(GLFW_KEY_W))            cam.processKeyboard(CameraMove::Forward,  dt, fast);
    if (down(GLFW_KEY_S))            cam.processKeyboard(CameraMove::Backward, dt, fast);
    if (down(GLFW_KEY_A))            cam.processKeyboard(CameraMove::Left,     dt, fast);
    if (down(GLFW_KEY_D))            cam.processKeyboard(CameraMove::Right,    dt, fast);
    if (down(GLFW_KEY_SPACE))        cam.processKeyboard(CameraMove::Up,       dt, fast);
    if (down(GLFW_KEY_LEFT_CONTROL)) cam.processKeyboard(CameraMove::Down,     dt, fast);
}

int main() {
    // ---- Инициализация на GLFW и прозорец ----
    if (!glfwInit()) { std::cerr << "GLFW init failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);                 // 4x MSAA - изглаждане на ръбовете
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(g_app.width, g_app.height,
                                          "Car Simulator", nullptr, nullptr);
    if (!window) { std::cerr << "Window creation failed\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD failed\n"; return -1;
    }
    std::cout << "OpenGL: " << glGetString(GL_VERSION)  << "\n"
              << "GPU:    " << glGetString(GL_RENDERER) << "\n";

    glfwGetFramebufferSize(window, &g_app.width, &g_app.height);
    glViewport(0, 0, g_app.width, g_app.height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);

    try {
        Shader shader(ASSET_DIR "shaders/basic.vert", ASSET_DIR "shaders/basic.frag");

        // ---- Текстури ----
        Texture grassTex(ASSET_DIR "textures/grass.png");
        Texture rockTex (ASSET_DIR "textures/rock.png");
        Texture roadTex (ASSET_DIR "textures/road.png");

        // ---- Писта: контролни точки на затворената крива (в метри, равнина XZ) ----
        const std::vector<glm::vec2> trackPoints = {
            {-120.0f,  -60.0f}, { -40.0f,  -95.0f}, {  60.0f,  -85.0f}, { 135.0f,  -45.0f},
            { 145.0f,   40.0f}, {  95.0f,   95.0f}, {  20.0f,   65.0f}, { -40.0f,  105.0f},
            {-120.0f,   85.0f}, {-155.0f,   10.0f},
        };
        Road road(trackPoints, 10.0f);              // ширина 10 м

        // ---- Терен 500 x 500 м, 250 x 250 клетки (по 2 м) ----
        Terrain terrain(500.0f, 250, road);

        std::cout << "Track length: " << road.length() << " m\n";

        // Камерата започва над началото на пистата и гледа по посоката ѝ
        glm::vec3 startDir = road.startDirection();
        float startYaw = glm::degrees(std::atan2(startDir.z, startDir.x));
        g_app.camera = Camera(road.startPosition() - startDir * 8.0f + glm::vec3(0, 3.0f, 0),
                              startYaw, -8.0f);

        // Свързваме sampler-ите в шейдъра с текстурните слотове (веднъж)
        shader.use();
        shader.setInt("uTexture",  0);
        shader.setInt("uTexture2", 1);

        // ---- Главен цикъл ----
        float lastFrame = 0.0f;
        float fpsTimer  = 0.0f;
        int   frames    = 0;

        while (!glfwWindowShouldClose(window)) {
            float now = static_cast<float>(glfwGetTime());
            float dt  = now - lastFrame;
            lastFrame = now;

            processInput(window, dt);

            // Камерата не може да слезе под терена: поне 0.5 м над земята
            glm::vec3 camPos = g_app.camera.position();
            float ground = terrain.heightAt(camPos.x, camPos.z) + 0.5f;
            if (camPos.y < ground) {
                camPos.y = ground;
                g_app.camera.setPosition(camPos);
            }

            fpsTimer += dt; ++frames;
            if (fpsTimer >= 1.0f) {
                char title[160];
                std::snprintf(title, sizeof(title),
                    "Car Simulator | FPS: %d | Cam: (%.1f, %.1f, %.1f) | Ground: %.1f m",
                    frames, camPos.x, camPos.y, camPos.z, ground - 0.5f);
                glfwSetWindowTitle(window, title);
                fpsTimer = 0.0f; frames = 0;
            }

            // ---- Рисуване ----
            glClearColor(0.62f, 0.78f, 0.95f, 1.0f);   // същият цвят като мъглата
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            float aspect = g_app.height > 0 ? (float)g_app.width / g_app.height : 1.0f;

            shader.use();
            shader.setMat4("uView",       g_app.camera.getViewMatrix());
            shader.setMat4("uProjection", g_app.camera.getProjectionMatrix(aspect));
            shader.setVec3("uCameraPos",  g_app.camera.position());
            shader.setMat4("uModel",      glm::mat4(1.0f));   // теренът и пътят са в световни координати
            shader.setVec3("uColor",      glm::vec3(1.0f));

            // Терен: трева (слот 0) + скала (слот 1)
            grassTex.bind(0);
            rockTex.bind(1);
            shader.setInt("uMode", MODE_TERRAIN);
            terrain.draw();

            // Път: изместване на дълбочината (polygon offset), за да не "трепти"
            // срещу терена под него (z-fighting)
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-1.0f, -1.0f);
            roadTex.bind(0);
            shader.setInt("uMode", MODE_TEXTURE);
            road.draw();
            glDisable(GL_POLYGON_OFFSET_FILL);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        glfwTerminate();
        return -1;
    }

    glfwTerminate();
    return 0;
}
