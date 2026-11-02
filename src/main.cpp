// ============================================================
//  Интерактивен 3D автомобилен симулатор
//  Етап 7: Ден и нощ - движещо се слънце, небесна сфера,
//          фарове (прожекторни светлини), лампи и прозорци нощем
//
//  Ден и нощ:
//    N            - следващ момент: обед -> залез -> нощ -> изгрев
//    T            - пауза / пускане на времето
//    [ / ]        - превъртане на времето назад / напред (задръж)
//    L            - фарове: автоматично -> включени -> изключени
//
//  Камери:
//    C            - смяна: трето лице -> над капака -> свободна -> ...
//
//  Управление на колата (стрелките работят винаги):
//    W / Стрелка нагоре   - газ
//    S / Стрелка надолу   - спирачка / заден ход
//    A, D / Стрелки       - волан
//    Space / H            - ръчна спирачка
//    R                    - връщане на старта
//    (в режим "свободна камера" WASD/Space управляват камерата, не колата)
//
//  Камера от трето лице:
//    Мишка        - оглеждане около колата (връща се сама зад нея)
//    Колелце      - приближаване / отдалечаване
//
//  Свободна камера:
//    W/A/S/D      - напред / наляво / назад / надясно
//    Space / Ctrl - нагоре / надолу
//    Shift        - по-бързо движение
//    Мишка        - оглеждане
//    Колелце      - приближаване (промяна на FOV)
//    1 / 2 / 3    - ambient / diffuse / specular компонента вкл./изкл.
//    4            - слънце / луна вкл./изкл.
//    5            - улични лампи разрешени / забранени (светят само нощем)
//    B            - Phong <-> Blinn-Phong
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
#include "Lighting.h"
#include "Renderer.h"
#include "Car.h"
#include "FollowCamera.h"
#include "World.h"
#include "DayNight.h"
#include "Sky.h"

// ------------------------------------------------------------
//  Глобално състояние на приложението (достъпно от callback-ите)
// ------------------------------------------------------------
// Активна камера
enum class CameraMode { ThirdPerson, Hood, Free };

struct AppState {
    int    width = 1280, height = 720;
    Camera       camera;            // свободна камера
    FollowCamera follow;            // камера, следваща колата (трето лице / капак)
    CameraMode   cameraMode = CameraMode::ThirdPerson;
    bool         switchCamera = false;  // заявка за смяна (клавиш C)
    bool   firstMouse    = true;
    double lastX = 0.0, lastY = 0.0;
    bool   mouseCaptured = true;
    bool   wireframe     = false;
    LightingToggles light;          // кои компоненти на осветлението са включени
    DayNight        dayNight;       // час от денонощието и цветовете, които зависят от него
    int             headlightMode = 0;   // 0 - автоматично, 1 - включени, 2 - изключени
    bool   resetCar      = false;   // заявка за връщане на колата на старта (клавиш R)
};
static AppState g_app;

// ------------------------------------------------------------
//  Callback функции
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
    float yOffset = static_cast<float>(g_app.lastY - y);
    g_app.lastX = x;
    g_app.lastY = y;

    // Мишката управлява активната камера
    if (g_app.cameraMode == CameraMode::Free)
        g_app.camera.processMouseMovement(xOffset, yOffset);
    else if (g_app.cameraMode == CameraMode::ThirdPerson)
        g_app.follow.addOrbit(xOffset * 0.15f, -yOffset * 0.15f);
}

static void scrollCallback(GLFWwindow*, double, double yOffset) {
    if (g_app.cameraMode == CameraMode::Free)
        g_app.camera.processMouseScroll(static_cast<float>(yOffset));
    else
        g_app.follow.zoom(static_cast<float>(yOffset));
}

static void keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    LightingToggles& l = g_app.light;

    switch (key) {
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, true); break;
        case GLFW_KEY_TAB:
            g_app.mouseCaptured = !g_app.mouseCaptured;
            glfwSetInputMode(window, GLFW_CURSOR,
                             g_app.mouseCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            g_app.firstMouse = true;
            break;
        case GLFW_KEY_F1:
            g_app.wireframe = !g_app.wireframe;
            glPolygonMode(GL_FRONT_AND_BACK, g_app.wireframe ? GL_LINE : GL_FILL);
            break;
        // Демонстрация на компонентите на модела на Phong
        case GLFW_KEY_1: l.ambient  = !l.ambient;  break;
        case GLFW_KEY_2: l.diffuse  = !l.diffuse;  break;
        case GLFW_KEY_3: l.specular = !l.specular; break;
        case GLFW_KEY_4: l.sun      = !l.sun;      break;
        case GLFW_KEY_5: l.lamps    = !l.lamps;    break;
        case GLFW_KEY_B: l.blinn    = !l.blinn;    break;
        case GLFW_KEY_R: g_app.resetCar = true;    break;
        case GLFW_KEY_C: g_app.switchCamera = true; break;
        case GLFW_KEY_N: g_app.dayNight.nextPreset();  break;
        case GLFW_KEY_T: g_app.dayNight.togglePause(); break;
        case GLFW_KEY_L: g_app.headlightMode = (g_app.headlightMode + 1) % 3; break;
        default: break;
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

// ------------------------------------------------------------
//  Команди към колата от клавиатурата.
//  Клавиатурата дава 0 или 1; Car сам изглажда волана.
// ------------------------------------------------------------
static CarInput readCarInput(GLFWwindow* window) {
    auto down = [&](int key) { return glfwGetKey(window, key) == GLFW_PRESS; };
    // WASD управляват колата само когато свободната камера НЕ е активна
    const bool wasd = g_app.cameraMode != CameraMode::Free;

    bool gas   = down(GLFW_KEY_UP)    || (wasd && down(GLFW_KEY_W));
    bool brake = down(GLFW_KEY_DOWN)  || (wasd && down(GLFW_KEY_S));
    bool left  = down(GLFW_KEY_LEFT)  || (wasd && down(GLFW_KEY_A));
    bool right = down(GLFW_KEY_RIGHT) || (wasd && down(GLFW_KEY_D));

    CarInput in;
    in.throttle  = gas   ? 1.0f : 0.0f;
    in.brake     = brake ? 1.0f : 0.0f;
    in.steer     = (right ? 1.0f : 0.0f) - (left ? 1.0f : 0.0f);
    in.handbrake = down(GLFW_KEY_H) || (wasd && down(GLFW_KEY_SPACE));
    return in;
}

// ------------------------------------------------------------
//  Смяна на камерата: трето лице -> капак -> свободна -> трето лице
// ------------------------------------------------------------
static void cycleCamera(const Car& car) {
    switch (g_app.cameraMode) {
        case CameraMode::ThirdPerson:
            g_app.cameraMode  = CameraMode::Hood;
            g_app.follow.mode = FollowMode::Hood;
            break;

        case CameraMode::Hood: {
            // Свободната камера започва ТОЧНО там, където е била следващата,
            // за да няма "скок" в картината. Ъглите yaw/pitch се изчисляват
            // обратно от посоката на гледане (обратното на Camera::updateVectors).
            glm::vec3 pos = g_app.follow.position();
            glm::vec3 dir = glm::normalize(g_app.follow.target() - pos);
            float yaw   = glm::degrees(std::atan2(dir.z, dir.x));
            float pitch = glm::degrees(std::asin(glm::clamp(dir.y, -1.0f, 1.0f)));
            g_app.camera = Camera(pos, yaw, pitch);
            g_app.cameraMode = CameraMode::Free;
            break;
        }

        case CameraMode::Free:
            g_app.cameraMode  = CameraMode::ThirdPerson;
            g_app.follow.mode = FollowMode::ThirdPerson;
            g_app.follow.snapTo(car);
            break;
    }
    g_app.firstMouse = true;
}

static const char* cameraName(CameraMode m) {
    switch (m) {
        case CameraMode::ThirdPerson: return "3rd person";
        case CameraMode::Hood:        return "Hood";
        default:                      return "Free";
    }
}

int main() {
    // ---- Инициализация на GLFW и прозорец ----
    if (!glfwInit()) { std::cerr << "GLFW init failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
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

        // ---- Геометрия ----
        std::unique_ptr<Mesh> cube  (Primitives::createCube());
        std::unique_ptr<Mesh> cylinder(Primitives::createCylinder());
        std::unique_ptr<Mesh> quad  (Primitives::createPlane(1.0f));   // за сянката на колата

        const std::vector<glm::vec2> trackPoints = {
            {-120.0f,  -60.0f}, { -40.0f,  -95.0f}, {  60.0f,  -85.0f}, { 135.0f,  -45.0f},
            { 145.0f,   40.0f}, {  95.0f,   95.0f}, {  20.0f,   65.0f}, { -40.0f,  105.0f},
            {-120.0f,   85.0f}, {-155.0f,   10.0f},
        };
        Road    road(trackPoints, 10.0f);
        Terrain terrain(500.0f, 250, road);

        // ---- Материали ----
        Material grassMat;  grassMat.specular = 0.05f; grassMat.shininess = 8.0f;
        Material roadMat;   roadMat.specular  = 0.25f; roadMat.shininess  = 24.0f;

        // ---- Небе ----
        Sky sky;

        // ---- Обекти покрай пистата: дървета, сгради, лампи, мантинели, конуси ----
        World world(road, terrain);
        std::cout << "Track length: " << road.length() << " m, trees: " << world.treeCount()
                  << ", lamps: " << world.lampLights().size() << "\n";
        int crashes = 0;     // брой силни удари

        // ---- Кола на старта на пистата ----
        glm::vec3 startDir     = road.startDirection();
        float     startHeading = std::atan2(startDir.z, startDir.x);   // в радиани
        Car car(road.startPosition(), startHeading);

        // Камерата от трето лице - веднага зад колата
        g_app.follow.snapTo(car);
        g_app.follow.update(car, terrain, 0.0f);

        shader.use();
        shader.setInt("uTexture",  0);
        shader.setInt("uTexture2", 1);

        // ---- Главен цикъл ----
        float lastFrame = 0.0f;
        float fpsTimer  = 0.0f;
        int   frames    = 0;

        while (!glfwWindowShouldClose(window)) {
            float now = static_cast<float>(glfwGetTime());
            float frameTime = now - lastFrame;   // реалното време на кадъра (за FPS)
            lastFrame = now;
            // Ограничение на dt: ако програмата "замръзне" за момент (напр. при
            // влачене на прозореца), физиката не трябва да прави огромна стъпка
            float dt = frameTime > 0.05f ? 0.05f : frameTime;

            if (g_app.switchCamera) {
                cycleCamera(car);
                g_app.switchCamera = false;
            }

            // ---- Физика на колата ----
            if (g_app.resetCar) {
                car.reset(road.startPosition(), startHeading);
                world.reset();
                g_app.follow.snapTo(car);
                g_app.resetCar = false;
            }
            car.update(dt, readCarInput(window), terrain, road);

            // ---- Колизии: СЛЕД движението, ПРЕДИ рисуването ----
            float impact = world.collide(car);
            if (impact > 3.0f) {                         // силен удар (> ~11 km/h)
                ++crashes;
                g_app.follow.addShake(impact / 15.0f);
            }
            world.update(dt);

            // ---- Ден и нощ ----
            if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) g_app.dayNight.scrub( 3.0f * frameTime);
            if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET)  == GLFW_PRESS) g_app.dayNight.scrub(-3.0f * frameTime);
            g_app.dayNight.update(dt);

            const float night       = g_app.dayNight.nightFactor();
            const bool  lampsOn     = g_app.light.lamps && night > 0.3f;
            const bool  headlightsOn = g_app.headlightMode == 1 ||
                                      (g_app.headlightMode == 0 && night > 0.3f);

            // ---- Камери ----
            // Следващата камера се обновява СЛЕД колата - използва новата ѝ позиция
            g_app.follow.update(car, terrain, dt);

            if (g_app.cameraMode == CameraMode::Free) {
                processInput(window, dt);
                glm::vec3 camPos = g_app.camera.position();
                float ground = terrain.heightAt(camPos.x, camPos.z) + 0.5f;
                if (camPos.y < ground) {
                    camPos.y = ground;
                    g_app.camera.setPosition(camPos);
                }
            }

            fpsTimer += frameTime; ++frames;   // FPS - по реалното време, не по dt
            if (fpsTimer >= 1.0f) {
                static const char* hlNames[3] = {"auto", "on", "off"};
                float hr = g_app.dayNight.hour();
                char title[220];
                std::snprintf(title, sizeof(title),
                    "Car Simulator | FPS: %d | %02d:%02d%s | Speed: %.0f km/h | Crashes: %d | "
                    "Headlights: %s | Camera: %s",
                    frames, int(hr), int((hr - int(hr)) * 60.0f),
                    g_app.dayNight.paused() ? " (paused)" : "",
                    car.speedKmh(), crashes, hlNames[g_app.headlightMode],
                    cameraName(g_app.cameraMode));
                glfwSetWindowTitle(window, title);
                fpsTimer = 0.0f; frames = 0;
            }

            // ---- Рисуване ----
            const glm::vec3 fog = g_app.dayNight.horizonColor();   // мъглата = цвета на хоризонта
            glClearColor(fog.r, fog.g, fog.b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            float aspect = g_app.height > 0 ? (float)g_app.width / g_app.height : 1.0f;

            // View, Projection и позиция - от активната камера
            glm::mat4 view, projection;
            glm::vec3 eye;
            if (g_app.cameraMode == CameraMode::Free) {
                view       = g_app.camera.getViewMatrix();
                projection = g_app.camera.getProjectionMatrix(aspect);
                eye        = g_app.camera.position();
            } else {
                view       = g_app.follow.getViewMatrix();
                projection = g_app.follow.getProjectionMatrix(aspect);
                eye        = g_app.follow.position();
            }

            shader.use();
            shader.setMat4("uView",       view);
            shader.setMat4("uProjection", projection);
            shader.setVec3("uCameraPos",  eye);
            shader.setVec3("uFogColor",   fog);

            // Светлини: слънце/луна, лампите (само нощем), фаровете (ако са включени)
            static const std::vector<PointLight> noLamps;
            std::vector<SpotLight> spots;
            if (headlightsOn) {
                SpotLight left, right;
                car.headlights(left, right);
                spots = {left, right};
            }
            uploadLights(shader, g_app.dayNight.light(),
                         lampsOn ? world.lampLights() : noLamps, spots, g_app.light);

            // Терен
            grassTex.bind(0);
            rockTex.bind(1);
            {
                // Теренът няма Model трансформация - използваме drawMesh с единична матрица
                shader.setMat4("uModel", glm::mat4(1.0f));
                shader.setMat3("uNormalMatrix", glm::mat3(1.0f));
                uploadMaterial(shader, grassMat);
                shader.setInt("uMode", MODE_TERRAIN);
                terrain.draw();
            }

            // Път (с polygon offset срещу z-fighting)
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-1.0f, -1.0f);
            roadTex.bind(0);
            shader.setMat4("uModel", glm::mat4(1.0f));
            shader.setMat3("uNormalMatrix", glm::mat3(1.0f));
            uploadMaterial(shader, roadMat);
            shader.setInt("uMode", MODE_TEXTURE);
            road.draw();
            glDisable(GL_POLYGON_OFFSET_FILL);

            // Дървета, сгради, лампи, мантинели, бали, конуси
            world.draw(shader, lampsOn, night);

            // Небето - след непрозрачните обекти: пикселите, вече закрити от
            // терена/дърветата, не минават теста за дълбочина и не се изчисляват
            sky.draw(view, projection, g_app.dayNight);

            // Колата - последна, защото сянката ѝ е полупрозрачна и трябва
            // да се смеси с вече нарисувания път/терен под нея.
            // sky.draw() смени шейдъра, затова отново активираме основния.
            shader.use();
            car.draw(shader, *cube, *cylinder, *quad, headlightsOn);

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
