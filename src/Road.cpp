#include "Road.h"

#include <algorithm>
#include <cmath>
#include <limits>

// ------------------------------------------------------------
//  Сплайн на Catmull-Rom: сегментът между P1 и P2 зависи и от
//  съседните точки P0 и P3, които задават наклона (тангентата).
//  При t = 0 връща P1, при t = 1 връща P2.
// ------------------------------------------------------------
glm::vec2 Road::catmullRom(const glm::vec2& p0, const glm::vec2& p1,
                           const glm::vec2& p2, const glm::vec2& p3, float t) {
    float t2 = t * t, t3 = t2 * t;
    return 0.5f * ( 2.0f * p1
                  + (-p0 + p2) * t
                  + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2
                  + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3 );
}

Road::Road(const std::vector<glm::vec2>& cp, float width, int samplesPerSegment)
    : m_halfWidth(width * 0.5f) {

    // ---- 1. Дискретизация на затворената крива ----
    const int n = static_cast<int>(cp.size());
    for (int i = 0; i < n; ++i) {
        // Индексите се взимат по модул n -> кривата се затваря
        const glm::vec2& p0 = cp[(i - 1 + n) % n];
        const glm::vec2& p1 = cp[i];
        const glm::vec2& p2 = cp[(i + 1) % n];
        const glm::vec2& p3 = cp[(i + 2) % n];
        for (int s = 0; s < samplesPerSegment; ++s)
            m_center.push_back(catmullRom(p0, p1, p2, p3, float(s) / samplesPerSegment));
    }

    // ---- 2. Обща дължина и дължина на една текстура ----
    const int count = static_cast<int>(m_center.size());
    for (int i = 0; i < count; ++i)
        m_length += glm::length(m_center[(i + 1) % count] - m_center[i]);

    // Една текстура на пътя покрива ~12 м. Закръгляме така, че по цялата
    // дължина да се съберат ЦЯЛ брой повторения -> няма "шев" при затварянето.
    const float repeats    = std::round(m_length / 12.0f);
    const float tileLength = m_length / repeats;

    // ---- 3. Лента от върхове: за всяка точка - ляв и десен ръб ----
    std::vector<Vertex> verts;
    std::vector<unsigned int> idx;
    float travelled = 0.0f;

    for (int i = 0; i <= count; ++i) {               // <= : последната точка дублира първата
        const glm::vec2& c    = m_center[i % count];
        const glm::vec2& prev = m_center[(i - 1 + count) % count];
        const glm::vec2& next = m_center[(i + 1) % count];

        if (i > 0) travelled += glm::length(c - prev);

        // Тангента (посока на движение) и перпендикулярът към нея в XZ.
        // За посока (tx, tz) векторът (-tz, tx) сочи НАДЯСНО
        // (= cross(front, up), както при камерата).
        glm::vec2 tangent = glm::normalize(next - prev);
        glm::vec2 right(-tangent.y, tangent.x);

        glm::vec2 leftEdge  = c - right * m_halfWidth;
        glm::vec2 rightEdge = c + right * m_halfWidth;
        float v = travelled / tileLength;            // V расте по дължината на пътя

        // U = 0 в левия ръб, U = 1 в десния ръб
        verts.push_back({{leftEdge.x,  kSurfaceY, leftEdge.y }, {0, 1, 0}, {0.0f, v}});
        verts.push_back({{rightEdge.x, kSurfaceY, rightEdge.y}, {0, 1, 0}, {1.0f, v}});

        if (i < count) {
            // Четириъгълник между двойка i и двойка i+1 = два триъгълника.
            //   a+2 ---- a+3        (a = ляв ръб, a+1 = десен ръб на двойка i)
            //    |  \     |
            //    |    \   |         обход обратно на часовниковата стрелка,
            //    a ---- a+1         гледано отгоре
            unsigned int a = i * 2;
            idx.insert(idx.end(), {a, a + 1, a + 2,  a + 1, a + 3, a + 2});
        }
    }
    m_mesh = std::make_unique<Mesh>(verts, idx);
}

// ------------------------------------------------------------
//  Разстояние от точка P до полилинията = минимумът от
//  разстоянията до всяка отсечка AB. Проекцията на P върху AB
//  се ограничава в [0,1], за да остане в рамките на отсечката.
// ------------------------------------------------------------
float Road::distanceToCenter(float x, float z) const {
    glm::vec2 p(x, z);
    float best = std::numeric_limits<float>::max();
    const size_t count = m_center.size();
    for (size_t i = 0; i < count; ++i) {
        glm::vec2 a = m_center[i];
        glm::vec2 b = m_center[(i + 1) % count];
        glm::vec2 ab = b - a;
        float t = glm::dot(p - a, ab) / glm::dot(ab, ab);
        t = std::clamp(t, 0.0f, 1.0f);
        best = std::min(best, glm::length(p - (a + ab * t)));
    }
    return best;
}

glm::vec3 Road::startPosition() const {
    return {m_center[0].x, kSurfaceY, m_center[0].y};
}

glm::vec3 Road::startDirection() const {
    glm::vec2 d = glm::normalize(m_center[1] - m_center[0]);
    return {d.x, 0.0f, d.y};
}
