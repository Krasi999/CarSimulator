#pragma once
// ============================================================
//  Road - затворена писта, зададена с няколко контролни точки.
//
//  1) Централната линия е гладка крива на Catmull-Rom, която
//     минава ТОЧНО през контролните точки.
//  2) Кривата се дискретизира на много на брой точки (полилиния).
//  3) От всяка точка се взимат две точки встрани (наляво и надясно
//     на половин ширина) -> получава се лента от триъгълници
//     (triangle strip), върху която се опъва текстурата на асфалта.
// ============================================================
#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "Mesh.h"

class Road {
public:
    Road(const std::vector<glm::vec2>& controlPoints, float width,
         int samplesPerSegment = 24);

    void draw() const { m_mesh->draw(); }

    // Най-късото разстояние (в равнината XZ) от точка до централната линия
    float distanceToCenter(float x, float z) const;

    float halfWidth() const { return m_halfWidth; }
    float length()    const { return m_length; }

    // Начална позиция и посока - за поставяне на камерата (и колата по-късно)
    glm::vec3 startPosition()  const;
    glm::vec3 startDirection() const;

    static constexpr float kSurfaceY = 0.05f;   // пътят е малко над терена

private:
    static glm::vec2 catmullRom(const glm::vec2& p0, const glm::vec2& p1,
                                const glm::vec2& p2, const glm::vec2& p3, float t);

    std::vector<glm::vec2> m_center;   // точки по централната линия (затворена)
    float m_halfWidth;
    float m_length = 0.0f;
    std::unique_ptr<Mesh> m_mesh;
};
