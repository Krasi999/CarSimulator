// ============================================================
//  Интерактивен 3D автомобилен симулатор
//  Етап 1: Свободна камера (WASD + мишка) и перспективна проекция
//
//  Управление:
//    W/A/S/D      - напред / наляво / назад / надясно
//    Space / Ctrl - нагоре / надолу
//    Shift        - по-бързо движение
//    Мишка        - оглеждане
//    Колелце      - приближаване (промяна на FOV)
//    Tab          - освобождаване / прихващане на курсора
//    Esc          - изход
// ============================================================
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <cstdio>

#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"

// ------------------------------------------------------------
//  Глобално състояние на приложението (достъпно от callback-ите)
// ------------------------------------------------------------
struct AppState {
    int    width = 1280, height = 720;
    Camera camera;
    bool   firstMouse   = true;   // за да няма "скок" при първото движение
    double lastX = 0.0, lastY = 0.0;
    bool   mouseCaptured = true;
};
static AppState g_app;

// ------------------------------------------------------------
//  Callback функции - GLFW ги извиква при събития
// ------------------------------------------------------------
static void framebufferSizeCallback(GLFWwindow*, int w, int h) {
    g_app.width = w;
    g_app.height = h;
    glViewport(0, 0, w, h);
}

// Извиква се при всяко движение на мишката с абсолютната позиция
// на курсора. Ние ни трябва разликата (offset) спрямо предишната.
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

// Еднократни натискания (не задържане) се обработват тук
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
}

// Задържани клавиши се проверяват всеки кадър
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

// ------------------------------------------------------------
//  Помощна функция за рисуване на един обект
// ------------------------------------------------------------
static void drawObject(const Shader& shader, const Mesh& mesh,
                       const glm::mat4& model, const glm::vec3& color,
                       bool checker = false) {
    shader.setMat4("uModel", model);
    shader.setVec3("uColor", color);
    shader.setInt("uChecker", checker ? 1 : 0);
    mesh.draw();
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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // скрит и "заключен" курсор

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD failed\n"; return -1;
    }
    std::cout << "OpenGL: " << glGetString(GL_VERSION)  << "\n"
              << "GPU:    " << glGetString(GL_RENDERER) << "\n";

    glfwGetFramebufferSize(window, &g_app.width, &g_app.height);
    glViewport(0, 0, g_app.width, g_app.height);
    glEnable(GL_DEPTH_TEST);   // Z-буфер: по-близките обекти закриват по-далечните
    glEnable(GL_CULL_FACE);    // не рисуваме задните (невидими) стени на обектите
    glEnable(GL_MULTISAMPLE);

    try {
        Shader shader(ASSET_DIR "shaders/basic.vert", ASSET_DIR "shaders/basic.frag");

        std::unique_ptr<Mesh> cube (Primitives::createCube());
        std::unique_ptr<Mesh> plane(Primitives::createPlane(400.0f));

        // ---- Главен цикъл ----
        float lastFrame = 0.0f;
        float fpsTimer = 0.0f;
        int   frames = 0;

        while (!glfwWindowShouldClose(window)) {
            // deltaTime - времето от предишния кадър в секунди
            float now = static_cast<float>(glfwGetTime());
            float dt  = now - lastFrame;
            lastFrame = now;

            processInput(window, dt);

            // FPS и позиция на камерата в заглавието на прозореца (веднъж в секунда)
            fpsTimer += dt; ++frames;
            if (fpsTimer >= 1.0f) {
                const glm::vec3& p = g_app.camera.position();
                char title[128];
                std::snprintf(title, sizeof(title),
                              "Car Simulator | FPS: %d | Cam: (%.1f, %.1f, %.1f) | FOV: %.0f",
                              frames, p.x, p.y, p.z, g_app.camera.fov());
                glfwSetWindowTitle(window, title);
                fpsTimer = 0.0f; frames = 0;
            }

            // ---- Рисуване ----
            glClearColor(0.55f, 0.75f, 0.95f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            float aspect = g_app.height > 0 ? (float)g_app.width / g_app.height : 1.0f;

            shader.use();
            shader.setMat4("uView",       g_app.camera.getViewMatrix());
            shader.setMat4("uProjection", g_app.camera.getProjectionMatrix(aspect));
            shader.setVec3("uCameraPos",  g_app.camera.position());

            // Земя
            drawObject(shader, *plane, glm::mat4(1.0f), {0.35f, 0.60f, 0.30f}, true);

            // Временен "път" - дълга тъмна ивица (ще стане истински път в Етап 2)
            glm::mat4 road = glm::translate(glm::mat4(1.0f), {0.0f, 0.01f, 0.0f});
            road = glm::scale(road, {8.0f, 0.02f, 200.0f});
            drawObject(shader, *cube, road, {0.25f, 0.25f, 0.27f});

            // Кубове от двете страни - ориентири в пространството
            for (int i = -5; i <= 5; ++i) {
                float z = i * 15.0f;
                for (float x : {-12.0f, 12.0f}) {
                    float h = 2.0f + 1.5f * ((i + 7) % 3);   // различни височини
                    glm::mat4 m = glm::translate(glm::mat4(1.0f), {x, h * 0.5f, z});
                    m = glm::scale(m, {3.0f, h, 3.0f});
                    drawObject(shader, *cube, m, {0.80f, 0.55f, 0.35f});
                }
            }

            // Въртящ се куб в центъра - демонстрация на Model матрицата
            glm::mat4 spin = glm::translate(glm::mat4(1.0f), {0.0f, 1.5f, -10.0f});
            spin = glm::rotate(spin, now, {0.0f, 1.0f, 0.0f});
            drawObject(shader, *cube, spin, {0.20f, 0.60f, 0.90f});

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
