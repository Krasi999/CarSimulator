#include "DayNight.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float PI = 3.14159265358979f;

float smoothstep(float e0, float e1, float x) {
    float t = std::clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// ---- Ключови цветове ----
const glm::vec3 kDayZenith     (0.22f, 0.45f, 0.88f);
const glm::vec3 kDayHorizon    (0.66f, 0.80f, 0.95f);
const glm::vec3 kNightZenith   (0.005f, 0.008f, 0.025f);
const glm::vec3 kNightHorizon  (0.025f, 0.035f, 0.07f);
const glm::vec3 kSunsetHorizon (0.98f, 0.52f, 0.28f);

const glm::vec3 kNoonSun       (1.00f, 0.96f, 0.88f);
const glm::vec3 kSunsetSun     (1.00f, 0.48f, 0.18f);
const glm::vec3 kMoonLight     (0.13f, 0.16f, 0.26f);

const glm::vec3 kDayAmbient    (0.30f, 0.34f, 0.40f);
const glm::vec3 kNightAmbient  (0.035f, 0.045f, 0.08f);
}

void DayNight::update(float dt) {
    if (!m_paused) scrub(hoursPerSecond * dt);
    else           recompute();
}

void DayNight::scrub(float hours) {
    m_hour = std::fmod(m_hour + hours + 24.0f, 24.0f);   // остава в [0, 24)
    recompute();
}

void DayNight::nextPreset() {
    // Прескачане към следващия "интересен" час
    const float presets[4] = {12.0f, 17.75f, 23.0f, 6.25f};
    int next = 0;
    for (int i = 0; i < 4; ++i)
        if (std::abs(m_hour - presets[i]) < 0.5f) next = (i + 1) % 4;
    m_hour = presets[next];
    recompute();
}

void DayNight::recompute() {
    // ---- 1. Посока на слънцето ----
    // В 6:00 изгрява (ъгъл 0), в 12:00 е най-високо (PI/2), в 18:00 залязва (PI).
    // Дъгата е леко наклонена към юг (+Z), както е в северното полукълбо.
    float a = (m_hour - 6.0f) / 12.0f * PI;
    m_sunDir = glm::normalize(glm::vec3(std::cos(a), std::sin(a) * 0.9f, 0.4f));

    // h - височина на слънцето: > 0 над хоризонта, < 0 под него
    const float h = m_sunDir.y;

    // ---- 2. Коефициенти за смесване ----
    float day    = smoothstep(-0.10f, 0.25f, h);                    // 0 нощ .. 1 ден
    float sunset = std::clamp(1.0f - std::abs(h - 0.05f) / 0.25f, 0.0f, 1.0f); // пик при хоризонта
    m_night      = 1.0f - smoothstep(-0.05f, 0.12f, h);

    // ---- 3. Цветове на небето ----
    m_zenith  = glm::mix(kNightZenith, kDayZenith, day);
    m_horizon = glm::mix(kNightHorizon, kDayHorizon, day);
    m_horizon = glm::mix(m_horizon, kSunsetHorizon, sunset * 0.7f);

    // ---- 4. Светлина: слънце (изчезва под хоризонта) и луна ----
    m_sunColor  = glm::mix(kNoonSun, kSunsetSun, sunset) * smoothstep(-0.05f, 0.08f, h);
    m_moonColor = kMoonLight * smoothstep(0.0f, -0.15f, h);

    // ---- 5. Фоново осветяване ----
    m_ambient = glm::mix(kNightAmbient, kDayAmbient, day) + glm::vec3(0.06f, 0.02f, 0.0f) * sunset;
}

DirLight DayNight::light() const {
    // Над хоризонта светлината идва от слънцето, под него - от луната.
    // В момента на смяната и двете са почти 0 -> няма видим "скок".
    DirLight l;
    bool sunUp  = m_sunDir.y > 0.0f;
    l.direction = sunUp ? m_sunDir : -m_sunDir;
    l.color     = sunUp ? m_sunColor : m_moonColor;
    l.ambient   = m_ambient;
    return l;
}
