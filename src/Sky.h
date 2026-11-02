#pragma once
// ============================================================
//  Sky - небесна сфера (skydome).
//
//  Голяма сфера около камерата, оцветена от собствен шейдър:
//    - градиент от хоризонта към зенита
//    - слънчев диск с ореол, лунен диск
//    - звезди нощем (процедурни)
//
//  Две важни техники:
//    1) От View матрицата се маха транслацията -> небето "върви"
//       с камерата и никога не може да се достигне.
//    2) gl_Position = pos.xyww -> дълбочината е винаги 1.0 (най-далеч),
//       така небето е зад всички останали обекти.
// ============================================================
#include <glm/glm.hpp>
#include <memory>

#include "Shader.h"
#include "Mesh.h"

class DayNight;

class Sky {
public:
    Sky();
    void draw(const glm::mat4& view, const glm::mat4& projection, const DayNight& dayNight) const;

private:
    std::unique_ptr<Shader> m_shader;
    std::unique_ptr<Mesh>   m_dome;
};
