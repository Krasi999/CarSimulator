#include "Collision.h"

#include <algorithm>
#include <cmath>
#include <limits>

bool boundingCirclesOverlap(const glm::vec2& a, float ra, const glm::vec2& b, float rb) {
    glm::vec2 d = b - a;
    float r = ra + rb;
    return glm::dot(d, d) <= r * r;   // сравняваме квадратите - без корен квадратен
}

// ------------------------------------------------------------
//  OBB срещу окръжност
//
//  1) Центърът на окръжността се изразява в ЛОКАЛНИТЕ координати
//     на правоъгълника (проекции върху неговите оси).
//  2) Ограничаваме тези координати в [-half, +half] -> получаваме
//     най-близката до окръжността точка от правоъгълника.
//  3) Ако разстоянието до нея е по-малко от радиуса -> колизия.
// ------------------------------------------------------------
Contact obbVsCircle(const OBB& box, const Circle& circle) {
    Contact c;
    const glm::vec2 ax = box.axisX(), az = box.axisZ();

    glm::vec2 d = circle.center - box.center;
    glm::vec2 local(glm::dot(d, ax), glm::dot(d, az));          // локални координати

    glm::vec2 clamped(std::clamp(local.x, -box.half.x, box.half.x),
                      std::clamp(local.y, -box.half.y, box.half.y));
    glm::vec2 closest = box.center + ax * clamped.x + az * clamped.y;

    glm::vec2 diff = circle.center - closest;
    float dist2 = glm::dot(diff, diff);
    if (dist2 > circle.radius * circle.radius) return c;          // няма колизия

    c.hit = true;
    if (dist2 > 1e-8f) {
        // Обичаен случай: центърът е ИЗВЪН правоъгълника.
        // Кутията се избутва в посока ОТ окръжността към нея.
        float dist = std::sqrt(dist2);
        c.normal = -diff / dist;
        c.depth  = circle.radius - dist;
    } else {
        // Центърът е ВЪТРЕ в правоъгълника - избутваме по оста,
        // по която проникването е най-малко.
        float px = box.half.x - std::abs(local.x);
        float pz = box.half.y - std::abs(local.y);
        if (px < pz) { c.normal = -ax * (local.x >= 0.0f ? 1.0f : -1.0f); c.depth = px + circle.radius; }
        else         { c.normal = -az * (local.y >= 0.0f ? 1.0f : -1.0f); c.depth = pz + circle.radius; }
    }
    return c;
}

// ------------------------------------------------------------
//  OBB срещу OBB - теорема за разделящата ос (Separating Axis Theorem)
//
//  Два изпъкнали многоъгълника НЕ се пресичат точно тогава, когато
//  съществува права (ос), върху която проекциите им не се застъпват.
//  За правоъгълници е достатъчно да се проверят 4 оси - нормалите
//  на страните им (по 2 от всеки).
//
//  Ако на всички 4 оси проекциите се застъпват -> колизия.
//  Оста с НАЙ-МАЛКО застъпване дава посоката и дълбочината на
//  най-краткото избутване (Minimum Translation Vector).
// ------------------------------------------------------------
Contact obbVsObb(const OBB& a, const OBB& b) {
    Contact c;
    const glm::vec2 axes[4] = { a.axisX(), a.axisZ(), b.axisX(), b.axisZ() };
    const glm::vec2 centerDiff = b.center - a.center;

    float minOverlap = std::numeric_limits<float>::max();
    glm::vec2 bestAxis(0.0f);

    for (const glm::vec2& L : axes) {
        // "Радиус" на проекцията на всеки правоъгълник върху оста L:
        // сумата от проекциите на половинките на страните му
        float ra = a.half.x * std::abs(glm::dot(a.axisX(), L)) + a.half.y * std::abs(glm::dot(a.axisZ(), L));
        float rb = b.half.x * std::abs(glm::dot(b.axisX(), L)) + b.half.y * std::abs(glm::dot(b.axisZ(), L));
        float dist = glm::dot(centerDiff, L);                    // разстояние между центровете по L

        float overlap = ra + rb - std::abs(dist);
        if (overlap <= 0.0f) return c;                           // разделяща ос -> няма колизия

        if (overlap < minOverlap) {
            minOverlap = overlap;
            // Посоката е такава, че да избута A ОТ B
            bestAxis = dist > 0.0f ? -L : L;
        }
    }
    c.hit    = true;
    c.normal = bestAxis;
    c.depth  = minOverlap;
    return c;
}
