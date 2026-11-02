#include "Car.h"
#include "Shader.h"
#include "Mesh.h"
#include "Terrain.h"
#include "Road.h"
#include "Lighting.h"
#include "Renderer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace {
// ---- Физични параметри (SI единици) ----
constexpr float kMass          = 1200.0f;   // kg
constexpr float kEngineForce   = 6000.0f;   // N   - максимална теглителна сила
constexpr float kBrakeForce    = 12000.0f;  // N
constexpr float kReverseForce  = 3000.0f;   // N
constexpr float kMaxReverse    = 8.0f;      // m/s - максимална скорост на заден ход
constexpr float kDrag          = 2.5f;      // въздушно съпротивление  F = kDrag * v^2
constexpr float kRollRoad      = 30.0f;     // триене при търкаляне по асфалт  F = k * v
constexpr float kRollGrass     = 300.0f;    // ... по трева - 10 пъти повече
constexpr float kGravity       = 9.81f;     // m/s^2

constexpr float kMaxSteer      = glm::radians(35.0f);  // максимален ъгъл на колелата
constexpr float kSteerRate     = 2.5f;      // rad/s - колко бързо се върти воланът
constexpr float kMaxLatAccel   = 14.0f;     // m/s^2 - максимално странично ускорение

constexpr float kWorldLimit    = 235.0f;    // колата не може да излезе от терена
}

Car::Car(const glm::vec3& position, float heading) {
    reset(position, heading);
}

void Car::reset(const glm::vec3& position, float heading) {
    m_position  = position;
    m_heading   = heading;
    m_speed     = 0.0f;
    m_steer     = 0.0f;
    m_forward   = {std::cos(heading), 0.0f, std::sin(heading)};
    m_up        = {0.0f, 1.0f, 0.0f};
    m_right     = glm::cross(m_forward, m_up);
}

// ============================================================
//  Физика - извиква се всеки кадър с времето dt от предишния
// ============================================================
void Car::update(float dt, const CarInput& in, const Terrain& terrain, const Road& road) {
    const float v = m_speed;

    // ---- 1. Волан ----
    // При висока скорост максималният ъгъл намалява, така че страничното
    // ускорение v^2 / R да не надхвърля kMaxLatAccel (иначе колата би
    // завивала "на място" при 150 km/h). От R = L / tan(δ) следва:
    //     tan(δmax) = L * aLat / v^2
    float maxSteer = kMaxSteer;
    if (std::abs(v) > 1.0f)
        maxSteer = std::min(kMaxSteer, std::atan(kWheelBase * kMaxLatAccel / (v * v)));

    // Воланът не "скача" - приближава целта с ограничена скорост
    float target = in.steer * maxSteer;
    float maxStep = kSteerRate * dt;
    m_steer += std::clamp(target - m_steer, -maxStep, maxStep);

    // ---- 2. Надлъжни сили ----
    float force = 0.0f;
    m_braking = false;

    if (in.throttle > 0.0f) {
        if (v >= -0.5f) force += kEngineForce * in.throttle;          // газ напред
        else          { force += kBrakeForce * in.throttle; m_braking = true; } // спира заден ход
    }
    if (in.brake > 0.0f) {
        if (v > 0.5f)              { force -= kBrakeForce * in.brake; m_braking = true; }
        else if (v > -kMaxReverse)   force -= kReverseForce * in.brake;   // заден ход
    }
    if (in.handbrake && std::abs(v) > 0.01f) {
        force -= (v > 0.0f ? 1.0f : -1.0f) * kBrakeForce * 1.2f;
        m_braking = true;
    }

    // Съпротивления - винаги срещу посоката на движение
    float roll = m_onRoad ? kRollRoad : kRollGrass;
    force -= kDrag * v * std::abs(v);    // въздух: ~ v^2
    force -= roll * v;                   // търкаляне: ~ v

    // 2-ри закон на Нютон + компонента на гравитацията по наклона.
    // m_forward.y = sin(ъгъл на наклона): нагоре -> забавя, надолу -> ускорява
    float accel = force / kMass - kGravity * m_forward.y;

    float newSpeed = v + accel * dt;     // интегриране по метода на Ойлер

    // Спирачката може да спре колата, но не и да я завърти назад
    if (m_braking && v * newSpeed < 0.0f) newSpeed = 0.0f;
    // Без команди и почти спряла -> спира напълно
    if (in.throttle == 0.0f && in.brake == 0.0f && std::abs(newSpeed) < 0.15f) newSpeed = 0.0f;
    m_speed = newSpeed;

    // ---- 3. Завиване: кинематичен модел на велосипеда ----
    // Двете предни колела се заменят с едно в средата, двете задни - също.
    // Задното колело се движи по окръжност с радиус R = L / tan(δ),
    // ъгловата скорост е  ω = v / R = v * tan(δ) / L
    float yawRate = m_speed * std::tan(m_steer) / kWheelBase;
    m_heading += yawRate * dt;

    // ---- 4. Преместване в хоризонталната равнина ----
    glm::vec3 flatForward(std::cos(m_heading), 0.0f, std::sin(m_heading));
    m_position += flatForward * m_speed * dt;

    // Граница на света (временно, до колизиите в Етап 6)
    if (std::abs(m_position.x) > kWorldLimit || std::abs(m_position.z) > kWorldLimit) {
        m_position.x = std::clamp(m_position.x, -kWorldLimit, kWorldLimit);
        m_position.z = std::clamp(m_position.z, -kWorldLimit, kWorldLimit);
        m_speed = 0.0f;
    }

    // ---- 5. Въртене на колелата: дъга = ъгъл * радиус ----
    m_wheelSpin += m_speed * dt / kWheelRadius;

    // ---- 6. Настилка и релеф ----
    m_onRoad = road.distanceToCenter(m_position.x, m_position.z) <= road.halfWidth() + 0.5f;
    followTerrain(dt, terrain);
}

// ------------------------------------------------------------
//  Следване на терена: височината се взима в 4 точки (отпред,
//  отзад, ляво, дясно). От тях се получават два вектора по
//  повърхността, а нормалата е тяхното векторно произведение.
// ------------------------------------------------------------
void Car::followTerrain(float dt, const Terrain& terrain) {
    glm::vec3 flatF(std::cos(m_heading), 0.0f, std::sin(m_heading));
    glm::vec3 flatR(-flatF.z, 0.0f, flatF.x);

    auto ground = [&](const glm::vec3& p) {
        // Пътят е 5 см над терена - колата не трябва да "потъва" в него
        return glm::vec3(p.x, std::max(terrain.heightAt(p.x, p.z), 0.05f), p.z);
    };
    glm::vec3 front = ground(m_position + flatF * (kWheelBase * 0.5f));
    glm::vec3 back  = ground(m_position - flatF * (kWheelBase * 0.5f));
    glm::vec3 right = ground(m_position + flatR * kTrackHalf);
    glm::vec3 left  = ground(m_position - flatR * kTrackHalf);

    m_position.y = (front.y + back.y + left.y + right.y) * 0.25f;

    glm::vec3 targetUp = glm::normalize(glm::cross(right - left, front - back));

    // Плавно доближаване към целевата нормала (експоненциално изглаждане)
    // -> колата не "трепери" по неравностите. Формулата 1 - e^(-k*dt)
    // дава еднакво поведение при всякакъв FPS.
    float blend = 1.0f - std::exp(-10.0f * dt);
    m_up = glm::normalize(glm::mix(m_up, targetUp, blend));

    // Ортонормиран базис: forward е flatF, "проектиран" върху наклона
    m_forward = glm::normalize(flatF - m_up * glm::dot(flatF, m_up));
    m_right   = glm::normalize(glm::cross(m_forward, m_up));
}

// ============================================================
//  Колизии
// ============================================================
OBB Car::collisionBox() const {
    return { glm::vec2(m_position.x, m_position.z),
             glm::vec2(kLength * 0.5f, kWidth * 0.5f),
             m_heading };
}

glm::vec2 Car::velocity2D() const {
    return glm::vec2(std::cos(m_heading), std::sin(m_heading)) * m_speed;
}

// ------------------------------------------------------------
//  Реакция на удар:
//   1) Позиционна корекция - избутваме колата навън по нормалата,
//      точно на дълбочината на проникване.
//   2) Импулс - компонентата на скоростта ПО нормалата (към
//      препятствието) се обръща и намалява с коеф. на възстановяване e:
//          v' = v - (1 + e) * (v · n) * n
//      e = 0 -> пълно "залепване", e = 1 -> идеално еластичен удар.
//      Компонентата ПО ДОПИРАТЕЛНАТА се запазва -> колата се плъзга
//      покрай стената при удар под малък ъгъл.
//   3) Колата може да се движи само напред/назад по посоката си,
//      затова новата скорост е проекцията на v' върху тази посока.
// ------------------------------------------------------------
float Car::applyCollision(const glm::vec2& normal, float depth, float restitution) {
    m_position.x += normal.x * depth;
    m_position.z += normal.y * depth;

    glm::vec2 v  = velocity2D();
    float     vn = glm::dot(v, normal);       // < 0 -> движим се КЪМ препятствието
    if (vn >= 0.0f) return 0.0f;              // вече се отдалечаваме - само корекцията

    v -= (1.0f + restitution) * vn * normal;

    glm::vec2 fwd(std::cos(m_heading), std::sin(m_heading));
    m_speed = glm::dot(v, fwd);
    return -vn;                               // сила на удара
}

glm::mat4 Car::modelMatrix() const {
    // Колоните на матрицата = образите на локалните оси (както при лампите)
    glm::mat4 m(1.0f);
    m[0] = glm::vec4(m_forward,  0.0f);   // локално X -> напред
    m[1] = glm::vec4(m_up,       0.0f);   // локално Y -> нагоре
    m[2] = glm::vec4(m_right,    0.0f);   // локално Z -> надясно
    m[3] = glm::vec4(m_position, 1.0f);
    return m;
}

// ============================================================
//  Йерархичен модел:
//    Свят
//     └─ Кола (modelMatrix)
//         ├─ каросерия, кабина, покрив, спойлер, светлини ...
//         └─ 4 x Колело (позиция -> завиване -> въртене)
//              ├─ гума
//              └─ джанта (2 спици)
// ============================================================
// ------------------------------------------------------------
//  Фарове: позицията е в локалните координати на колата (предната
//  броня), превърната в световна с матрицата на колата. Посоката е
//  "напред", наклонена леко надолу към пътя (както при истинските фарове).
// ------------------------------------------------------------
void Car::headlights(SpotLight& left, SpotLight& right) const {
    const glm::mat4 m = modelMatrix();
    glm::vec3 dir = glm::normalize(m_forward - m_up * 0.07f);   // ~4° надолу

    left.position  = glm::vec3(m * glm::vec4(2.25f, 0.68f, -0.62f, 1.0f));
    right.position = glm::vec3(m * glm::vec4(2.25f, 0.68f,  0.62f, 1.0f));
    left.direction = right.direction = dir;
}

void Car::draw(const Shader& shader, const Mesh& cube, const Mesh& cylinder,
               const Mesh& plane, bool lightsOn) const {
    const glm::mat4 car = modelMatrix();

    // ---- Материали ----
    Material paint;   paint.color  = {0.08f, 0.42f, 0.78f}; paint.specular = 0.7f;  paint.shininess = 96.0f;
    Material carbon;  carbon.color = {0.07f, 0.07f, 0.08f}; carbon.specular = 0.3f; carbon.shininess = 32.0f;
    Material glass;   glass.color  = {0.04f, 0.06f, 0.09f}; glass.specular = 1.0f;  glass.shininess = 128.0f;
    Material tire;    tire.color   = {0.04f, 0.04f, 0.04f}; tire.specular = 0.05f;  tire.shininess = 8.0f;
    Material rim;     rim.color    = {0.70f, 0.72f, 0.75f}; rim.specular = 0.8f;    rim.shininess = 64.0f;
    Material head;    head.color   = {0.9f, 0.9f, 0.85f};
    head.emissive = lightsOn ? glm::vec3(1.6f, 1.55f, 1.4f) : glm::vec3(0.35f, 0.35f, 0.32f);
    Material tail;    tail.color   = {0.5f, 0.02f, 0.02f};
    // Стопове: силно при спиране, по-слабо като габарити нощем
    tail.emissive = m_braking ? glm::vec3(1.2f, 0.05f, 0.05f)
                  : lightsOn  ? glm::vec3(0.6f, 0.02f, 0.02f)
                              : glm::vec3(0.25f, 0.0f, 0.0f);
    Material shadow;  shadow.color = glm::vec3(0.0f); shadow.specular = 0.0f;

    // ---- Сянка: полупрозрачен правоъгълник на земята под колата ----
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);   // alpha смесване
    glDepthMask(GL_FALSE);                               // не записваме в Z-буфера
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-2.0f, -2.0f);
    drawMesh(shader, plane, car * partTransform({0.0f, 0.03f, 0.0f}, {kLength + 0.6f, 1.0f, kWidth + 0.5f}),
             shadow, MODE_SHADOW);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // ---- Каросерия (координати: x - напред, y - нагоре, z - надясно) ----
    auto part = [&](const glm::vec3& c, const glm::vec3& s, const Material& m) {
        drawMesh(shader, cube, car * partTransform(c, s), m, MODE_COLOR);
    };
    part({ 0.00f, 0.60f, 0.0f}, {4.40f, 0.50f, 1.90f}, paint);    // основно тяло
    part({ 1.55f, 0.88f, 0.0f}, {1.30f, 0.06f, 1.80f}, paint);    // преден капак
    part({-0.30f, 1.08f, 0.0f}, {2.10f, 0.46f, 1.62f}, glass);    // кабина (стъкла)
    part({-0.35f, 1.33f, 0.0f}, {1.80f, 0.06f, 1.55f}, paint);    // покрив
    part({ 0.00f, 0.32f, 0.0f}, {4.30f, 0.12f, 1.86f}, carbon);   // долна част (праг)
    part({ 2.18f, 0.40f, 0.0f}, {0.12f, 0.22f, 1.86f}, carbon);   // предна броня
    part({-2.18f, 0.40f, 0.0f}, {0.12f, 0.22f, 1.86f}, carbon);   // задна броня / дифузьор

    // Спойлер: две стойки + крило
    part({-1.95f, 1.03f, -0.60f}, {0.10f, 0.40f, 0.06f}, carbon);
    part({-1.95f, 1.03f,  0.60f}, {0.10f, 0.40f, 0.06f}, carbon);
    part({-2.00f, 1.25f,  0.00f}, {0.38f, 0.05f, 2.00f}, carbon);

    // Фарове и стопове (emissive - светят сами)
    part({ 2.21f, 0.68f, -0.62f}, {0.03f, 0.12f, 0.38f}, head);
    part({ 2.21f, 0.68f,  0.62f}, {0.03f, 0.12f, 0.38f}, head);
    part({-2.21f, 0.72f, -0.62f}, {0.03f, 0.10f, 0.40f}, tail);
    part({-2.21f, 0.72f,  0.62f}, {0.03f, 0.10f, 0.40f}, tail);

    // ---- Колела ----
    const float axleX[2] = { kWheelBase * 0.5f, -kWheelBase * 0.5f };   // предна, задна ос
    const float sideZ[2] = { -kTrackHalf, kTrackHalf };                 // ляво, дясно

    for (int a = 0; a < 2; ++a) {
        for (int s = 0; s < 2; ++s) {
            // Ниво 1: позиция на колелото спрямо колата
            glm::mat4 wheel = glm::translate(car, {axleX[a], kWheelRadius, sideZ[s]});

            // Ниво 2: завиване - само предните колела, около вертикалната ос.
            // Положителен m_steer = завой надясно (към +Z), а завъртане около +Y
            // с положителен ъгъл обръща +X към -Z, затова знакът е минус.
            if (a == 0) wheel = glm::rotate(wheel, -m_steer, {0.0f, 1.0f, 0.0f});

            // Ниво 3: търкаляне - около оста на колелото (локално Z).
            // При движение напред горната точка на гумата отива напред (+X),
            // което е завъртане около +Z с ОТРИЦАТЕЛЕН ъгъл.
            wheel = glm::rotate(wheel, -m_wheelSpin, {0.0f, 0.0f, 1.0f});

            // Гума: цилиндърът е с ос Y -> завъртаме на 90° около X, за да е с ос Z
            glm::mat4 tireM = glm::rotate(wheel, glm::radians(90.0f), {1.0f, 0.0f, 0.0f});
            tireM = glm::scale(tireM, {kWheelRadius * 2.0f, kWheelWidth, kWheelRadius * 2.0f});
            drawMesh(shader, cylinder, tireM, tire, MODE_COLOR);

            // Джанта: две спици на външната страна - показват въртенето
            float outer = sideZ[s] > 0.0f ? 1.0f : -1.0f;
            float zOut  = outer * (kWheelWidth * 0.5f + 0.01f);
            drawMesh(shader, cube, wheel * partTransform({0.0f, 0.0f, zOut}, {0.50f, 0.08f, 0.02f}),
                     rim, MODE_COLOR);
            drawMesh(shader, cube, wheel * partTransform({0.0f, 0.0f, zOut}, {0.08f, 0.50f, 0.02f}),
                     rim, MODE_COLOR);
        }
    }
}
