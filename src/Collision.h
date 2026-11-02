#pragma once
// ============================================================
//  Collision - откриване на колизии в хоризонталната равнина (2D).
//
//  Колата се движи по земята, затова колизиите се проверяват в
//  проекция отгоре (координати X и Z). Формите са:
//    Circle - окръжност: стволове на дървета, стълбове, конуси, бали
//    OBB    - Oriented Bounding Box: правоъгълник, завъртян на
//             произволен ъгъл: колата, сгради, мантинели
//
//  Резултатът от проверката (Contact) казва:
//    hit    - има ли пресичане
//    normal - единичен вектор, в чиято посока трябва да се избута
//             ПЪРВИЯТ обект (колата), за да излезе от втория
//    depth  - колко дълбоко са проникнали един в друг (метри)
// ============================================================
#include <glm/glm.hpp>
#include <cmath>

struct Circle {
    glm::vec2 center;
    float     radius;
};

struct OBB {
    glm::vec2 center;
    glm::vec2 half;     // половин дължина (по локалното X) и половин ширина (по локалното Z)
    float     angle;    // ъгъл на завъртане в радиани (0 = локалното X съвпада със световното X)

    glm::vec2 axisX() const { return { std::cos(angle), std::sin(angle)}; }   // "напред"
    glm::vec2 axisZ() const { return {-std::sin(angle), std::cos(angle)}; }   // "надясно"
    float boundingRadius() const { return glm::length(half); }                // описана окръжност
};

struct Contact {
    bool      hit    = false;
    glm::vec2 normal {0.0f};
    float     depth  = 0.0f;
};

// Бърза предварителна проверка (broad phase): пресичат ли се описаните окръжности
bool boundingCirclesOverlap(const glm::vec2& a, float ra, const glm::vec2& b, float rb);

// Точни проверки (narrow phase)
Contact obbVsCircle(const OBB& box, const Circle& circle);
Contact obbVsObb(const OBB& a, const OBB& b);   // теорема за разделящата ос (SAT)
