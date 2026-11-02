// ============================================================
//  Интерактивен 3D автомобилен симулатор
//  Етап 5: Камера от трето лице, камера над капака и свободна камера
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
//    4            - слънце вкл./изкл.
//    5            - улични лампи вкл./изкл.
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

// ------------------------------------------------------------
//  Улична лампа - описание за рисуване
// ------------------------------------------------------------
struct StreetLamp {
    glm::mat4 transform;    // от локалните координати на лампата към света
};

// Части на лампата в ЛОКАЛНИ координати: основата е в (0,0,0),
// стълбът е по +Y, рамото излиза по +X (към пътя).
static const glm::vec3 kLampLightLocal(1.9f, 5.65f, 0.0f);   // къде е крушката

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
        std::unique_ptr<Mesh> sphere(Primitives::createSphere());
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
        Material metalMat;  metalMat.color = {0.35f, 0.37f, 0.40f};
                            metalMat.specular = 0.6f;  metalMat.shininess = 64.0f;
        Material bulbMat;   bulbMat.color = {1.0f, 0.95f, 0.8f};
                            bulbMat.specular = 0.0f;

        // ---- Слънце ----
        DirLight sun;

        // ---- Улични лампи покрай пътя ----
        // Поставят се през равен брой точки по централната линия, от
        // дясната страна на пътя, на 1.5 м след банкета.
        std::vector<StreetLamp> lamps;
        std::vector<PointLight> lampLights;
        const int lampEvery = road.sampleCount() / 12;          // 12 лампи по пистата
        for (int i = 0; i < road.sampleCount() && (int)lamps.size() < kMaxPointLights;
             i += lampEvery) {
            glm::vec3 center = road.centerPoint(i);
            glm::vec3 right  = road.rightVector(i);

            glm::vec3 base = center + right * (road.halfWidth() + 1.5f);
            base.y = terrain.heightAt(base.x, base.z);

            // Матрица на смяна на базиса: локалните оси на лампата -> световни вектори.
            //   локално X (рамото)  -> -right (към пътя)
            //   локално Y (стълбът) -> нагоре
            //   локално Z           -> X x Y (дясна координатна система)
            glm::vec3 axisX = -right;
            glm::vec3 axisY(0.0f, 1.0f, 0.0f);
            glm::vec3 axisZ = glm::cross(axisX, axisY);
            glm::mat4 m(1.0f);
            m[0] = glm::vec4(axisX, 0.0f);   // колона 0
            m[1] = glm::vec4(axisY, 0.0f);   // колона 1
            m[2] = glm::vec4(axisZ, 0.0f);   // колона 2
            m[3] = glm::vec4(base,  1.0f);   // колона 3 - транслация
            lamps.push_back({m});

            PointLight pl;
            pl.position = glm::vec3(m * glm::vec4(kLampLightLocal, 1.0f));
            lampLights.push_back(pl);
        }

        // ---- Сфери за демонстрация на материалите (до старта) ----
        // Еднакъв цвят, различна shininess -> различен размер на отблясъка.
        struct DemoSphere { glm::mat4 model; Material mat; };
        std::vector<DemoSphere> spheres;
        {
            glm::vec3 c0 = road.centerPoint(6);
            glm::vec3 r0 = road.rightVector(6);
            glm::vec3 d0 = road.direction(6);
            const float shin[3] = {8.0f, 32.0f, 128.0f};
            const glm::vec3 col[3] = {{0.80f, 0.15f, 0.12f}, {0.15f, 0.45f, 0.85f}, {0.90f, 0.70f, 0.20f}};
            for (int k = 0; k < 3; ++k) {
                glm::vec3 p = c0 - r0 * (road.halfWidth() + 3.0f) + d0 * (k * 5.0f);
                p.y = terrain.heightAt(p.x, p.z) + 1.0f;
                Material m; m.color = col[k]; m.specular = 0.8f; m.shininess = shin[k];
                spheres.push_back({glm::scale(glm::translate(glm::mat4(1.0f), p), glm::vec3(2.0f)), m});
            }
        }

        std::cout << "Track length: " << road.length() << " m, lamps: " << lamps.size() << "\n";

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
            float dt  = now - lastFrame;
            lastFrame = now;
            // Ограничение на dt: ако програмата "замръзне" за момент (напр. при
            // влачене на прозореца), физиката не трябва да прави огромна стъпка
            if (dt > 0.05f) dt = 0.05f;

            if (g_app.switchCamera) {
                cycleCamera(car);
                g_app.switchCamera = false;
            }

            // ---- Физика на колата ----
            if (g_app.resetCar) {
                car.reset(road.startPosition(), startHeading);
                g_app.follow.snapTo(car);
                g_app.resetCar = false;
            }
            car.update(dt, readCarInput(window), terrain, road);

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

            fpsTimer += dt; ++frames;
            if (fpsTimer >= 1.0f) {
                char title[200];
                std::snprintf(title, sizeof(title),
                    "Car Simulator | FPS: %d | Speed: %.0f km/h | Steer: %+.0f deg | %s | Camera: %s",
                    frames, car.speedKmh(), car.steerDeg(),
                    car.onRoad() ? "Road" : "Grass", cameraName(g_app.cameraMode));
                glfwSetWindowTitle(window, title);
                fpsTimer = 0.0f; frames = 0;
            }

            // ---- Рисуване ----
            glClearColor(0.62f, 0.78f, 0.95f, 1.0f);
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
            uploadLights(shader, sun, lampLights, g_app.light);

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

            // Улични лампи: всяка част = (матрица на лампата) * (локална матрица на частта)
            Material bulb = bulbMat;
            bulb.emissive = g_app.light.lamps ? glm::vec3(1.0f, 0.9f, 0.6f) : glm::vec3(0.0f);
            for (const StreetLamp& lamp : lamps) {
                drawMesh(shader, *cube, lamp.transform * partTransform({0.0f, 3.0f, 0.0f}, {0.18f, 6.0f, 0.18f}),
                         metalMat, MODE_COLOR);                                      // стълб
                drawMesh(shader, *cube, lamp.transform * partTransform({1.0f, 5.95f, 0.0f}, {2.0f, 0.12f, 0.12f}),
                         metalMat, MODE_COLOR);                                      // рамо
                drawMesh(shader, *cube, lamp.transform * partTransform({1.9f, 5.85f, 0.0f}, {0.7f, 0.15f, 0.35f}),
                         metalMat, MODE_COLOR);                                      // корпус
                drawMesh(shader, *cube, lamp.transform * partTransform({1.9f, 5.75f, 0.0f}, {0.55f, 0.06f, 0.25f}),
                         bulb, MODE_COLOR);                                          // светеща част
            }

            // Демонстрационни сфери
            for (const DemoSphere& s : spheres)
                drawMesh(shader, *sphere, s.model, s.mat, MODE_COLOR);

            // Колата - последна, защото сянката ѝ е полупрозрачна и трябва
            // да се смеси с вече нарисувания път/терен под нея
            car.draw(shader, *cube, *cylinder, *quad);

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
