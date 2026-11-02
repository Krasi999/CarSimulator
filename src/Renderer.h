#pragma once
// ============================================================
//  Renderer - общи помощни функции за рисуване на обекти.
//  Изнесени от main.cpp, защото вече ги ползват и други класове
//  (напр. Car рисува сам своите части).
// ============================================================
#include <glm/glm.hpp>

class Shader;
class Mesh;
struct Material;

// Режими на оцветяване - съвпадат с uMode във basic.frag
enum ShadeMode {
    MODE_COLOR   = 0,   // само цвят на материала
    MODE_TEXTURE = 1,   // текстура x цвят
    MODE_TERRAIN = 2,   // трева + скала според наклона
    MODE_SHADOW  = 3,   // мека полупрозрачна сянка под колата
};

// Рисува mesh с дадена Model матрица и материал.
// Изчислява и изпраща нормалната матрица (transpose(inverse(M))).
void drawMesh(const Shader& shader, const Mesh& mesh,
              const glm::mat4& model, const Material& material, int mode);

// Помощна: матрица на част от модел - преместване до center и мащабиране до size
glm::mat4 partTransform(const glm::vec3& center, const glm::vec3& size);
