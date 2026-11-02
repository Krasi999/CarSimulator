#include "FollowCamera.h"
#include "Car.h"
#include "Terrain.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace {
constexpr float PI = 3.14159265358979f;

// Привежда ъгъл в интервала [-PI, PI].
// Посоката на колата се натрупва (след 3 обиколки е ~6*PI), а камерата
// трябва да се завърти по КРАТКИЯ път, не да направи пълен кръг.
float wrapAngle(float a) {
    while (a >  PI) a -= 2.0f * PI;
    while (a < -PI) a += 2.0f * PI;
    return a;
}

// Коефициент за експоненциално изглаждане, независим от FPS:
// за 1 секунда стойността изминава една и съща част от разстоянието до целта.
float smoothFactor(float rate, float dt) {
    return 1.0f - std::exp(-rate * dt);
}

// ---- Настройки ----
constexpr float kYawFollowRate   = 4.0f;    // колко бързо камерата застава зад колата
constexpr float kHeightRate      = 6.0f;    // изглаждане на височината
constexpr float kFovRate         = 3.0f;
constexpr float kOrbitReturnRate = 1.2f;    // колко бързо се връща след оглеждане
constexpr float kBasePitch       = 14.0f;   // градуси над хоризонта
constexpr float kPivotHeight     = 1.1f;    // точката, около която обикаля камерата
constexpr float kLookAhead       = 3.0f;    // камерата гледа малко пред колата
constexpr float kMinAboveGround  = 0.6f;
}

void FollowCamera::snapTo(const Car& car) {
    m_yaw        = car.heading();
    m_carY       = car.position().y;
    m_orbitYaw   = 0.0f;
    m_orbitPitch = 0.0f;
}

void FollowCamera::addOrbit(float dYawDeg, float dPitchDeg) {
    m_orbitYaw   += dYawDeg;
    m_orbitPitch  = std::clamp(m_orbitPitch + dPitchDeg, -12.0f, 60.0f);
}

void FollowCamera::zoom(float scroll) {
    m_distance = std::clamp(m_distance - scroll * 0.7f, 4.0f, 16.0f);
}

void FollowCamera::update(const Car& car, const Terrain& terrain, float dt) {
    const float speedFactor = std::clamp(std::abs(car.speed()) / 40.0f, 0.0f, 1.0f);

    // ---- FOV расте със скоростта -> по-силно усещане за скорост ----
    float targetFov = (mode == FollowMode::Hood ? 68.0f : 60.0f) + 15.0f * speedFactor;
    m_fov += (targetFov - m_fov) * smoothFactor(kFovRate, dt);

    // ============================================================
    //  Камера над капака (първо лице)
    // ============================================================
    if (mode == FollowMode::Hood) {
        // Точка, зададена в ЛОКАЛНИТЕ координати на колата (x напред, y нагоре),
        // се превръща в световна с матрицата на колата - точно като частите на модела.
        glm::mat4 carM = car.modelMatrix();
        m_position = glm::vec3(carM * glm::vec4(0.75f, 1.30f, 0.0f, 1.0f));
        m_target   = m_position + car.forward() * 20.0f;
        m_up       = car.up();               // камерата се накланя заедно с колата
        m_yaw      = car.heading();          // за плавен преход при смяна на режима
        m_carY     = car.position().y;
        return;
    }

    // ============================================================
    //  Камера от трето лице
    // ============================================================

    // 1) Посоката на камерата "догонва" посоката на колата.
    //    Разликата се взима по краткия път (wrapAngle).
    float diff = wrapAngle(car.heading() - m_yaw);
    m_yaw += diff * smoothFactor(kYawFollowRate, dt);

    // 2) Височината се изглажда - камерата не подскача по неравностите
    m_carY += (car.position().y - m_carY) * smoothFactor(kHeightRate, dt);

    // 3) Оглеждането с мишката плавно отслабва -> камерата се връща зад колата
    float decay = std::exp(-kOrbitReturnRate * dt);
    m_orbitYaw   *= decay;
    m_orbitPitch *= decay;

    // 4) Сферични координати около pivot точката над колата
    float yaw      = m_yaw + glm::radians(m_orbitYaw);
    float pitch    = glm::radians(kBasePitch + m_orbitPitch);
    float distance = m_distance + 2.0f * speedFactor;   // при скорост - малко по-назад

    glm::vec3 pivot(car.position().x, m_carY + kPivotHeight, car.position().z);
    glm::vec3 dirFlat(std::cos(yaw), 0.0f, std::sin(yaw));          // хоризонтална посока "напред"

    // Камерата е НАЗАД по dirFlat и НАГОРЕ според pitch:
    //   хоризонтално разстояние = d * cos(pitch),  височина = d * sin(pitch)
    m_position = pivot - dirFlat * (distance * std::cos(pitch))
                       + glm::vec3(0.0f, distance * std::sin(pitch), 0.0f);

    // 5) Камерата не може да влезе в хълма зад колата
    float ground = terrain.heightAt(m_position.x, m_position.z) + kMinAboveGround;
    if (m_position.y < ground) m_position.y = ground;

    // 6) Гледа малко пред колата - вижда се повече от пътя напред
    m_target = pivot + dirFlat * kLookAhead;
    m_up     = glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::mat4 FollowCamera::getViewMatrix() const {
    return glm::lookAt(m_position, m_target, m_up);
}

glm::mat4 FollowCamera::getProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(m_fov), aspectRatio, 0.1f, 800.0f);
}
