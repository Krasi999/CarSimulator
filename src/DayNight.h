#pragma once
// ============================================================
//  DayNight - смяна на деня и нощта.
//
//  Пази текущия час (0..24) и от него изчислява:
//    - посоката на слънцето (движи се по дъга от изток на запад)
//      и на луната (в противоположната посока)
//    - цвета и силата на светлината: бяла по обед, оранжева при
//      изгрев/залез, слаба синкава лунна светлина нощем
//    - цвета на небето (зенит и хоризонт) и на мъглата
//    - фоновото (ambient) осветяване
//    - nightFactor: 0 = ден, 1 = нощ -> включва лампите, фаровете,
//      осветява прозорците
//
//  Всички цветове се получават чрез плавна интерполяция (mix /
//  smoothstep) според височината на слънцето над хоризонта.
// ============================================================
#include <glm/glm.hpp>

#include "Lighting.h"

class DayNight {
public:
    void update(float dt);

    // ---- Управление ----
    void togglePause()            { m_paused = !m_paused; }
    void nextPreset();                        // обед -> залез -> нощ -> изгрев -> обед
    void scrub(float hours);                  // превъртане напред/назад
    bool paused() const           { return m_paused; }
    float hour() const            { return m_hour; }

    // ---- Резултати (обновяват се в update) ----
    DirLight  light() const;                  // слънце или луна
    glm::vec3 sunDirection()  const { return m_sunDir; }
    glm::vec3 moonDirection() const { return -m_sunDir; }
    glm::vec3 sunColor()      const { return m_sunColor; }
    glm::vec3 zenithColor()   const { return m_zenith; }
    glm::vec3 horizonColor()  const { return m_horizon; }   // = цвят на мъглата
    float     nightFactor()   const { return m_night; }     // 0 ден .. 1 нощ

    float hoursPerSecond = 0.1f;              // 1 игрален час = 10 секунди

private:
    void recompute();

    float m_hour   = 11.0f;
    bool  m_paused = false;

    glm::vec3 m_sunDir{0.0f, 1.0f, 0.0f};
    glm::vec3 m_sunColor{1.0f};
    glm::vec3 m_moonColor{0.0f};
    glm::vec3 m_ambient{0.3f};
    glm::vec3 m_zenith{0.3f, 0.5f, 0.9f};
    glm::vec3 m_horizon{0.6f, 0.8f, 0.95f};
    float     m_night = 0.0f;
};
