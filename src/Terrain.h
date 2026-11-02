#pragma once
// ============================================================
//  Terrain - 3D терен, построен от height map (карта на височините).
//
//  Теренът е квадратна мрежа от (N+1) x (N+1) върха в равнината XZ.
//  Височината (Y) на всеки връх се изчислява процедурно с
//  фрактален шум (fBm). Близо до пътя теренът плавно се изравнява,
//  за да е пътят на равно, а по краищата на света се издигат
//  планини, които "затварят" сцената.
// ============================================================
#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "Mesh.h"

class Road;

class Terrain {
public:
    Terrain(float size, int resolution, const Road& road);

    void draw() const { m_mesh->draw(); }

    // Височина в произволна точка (x, z) - билинейна интерполация
    // между четирите най-близки върха на мрежата
    float heightAt(float x, float z) const;

    float size() const { return m_size; }

private:
    float gridHeight(int i, int j) const;    // височина на връх (i, j) от мрежата

    float m_size;        // дължина на страната в метри
    int   m_res;         // брой клетки по една страна
    float m_step;        // размер на една клетка
    std::vector<float> m_heights;
    std::unique_ptr<Mesh> m_mesh;
};
