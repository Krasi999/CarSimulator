#pragma once
// ============================================================
//  World - всички обекти около пистата и колизиите с тях.
//
//  Статични обекти (не се движат):
//    - дървета (2 вида, стотици копия -> инстанциране)
//    - сгради
//    - мантинели покрай част от завоите
//    - бали слама (защитна стена в края на правата)
//    - улични лампи (точкови светлини от Етап 3)
//    - декоративни сфери (демонстрация на материалите от Етап 3)
//  Динамични обекти:
//    - пътни конуси (слалом) - събарят се при удар
//
//  Всеки обект има форма за колизии (Circle или OBB).
// ============================================================
#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "Collision.h"
#include "Lighting.h"
#include "Mesh.h"

class Shader;
class Road;
class Terrain;
class Car;

class World {
public:
    World(const Road& road, const Terrain& terrain);

    void update(float dt);                          // анимация на съборените конуси
    void reset();                                   // връща конусите по местата им

    // Проверява и разрешава колизиите на колата с всички обекти.
    // Връща най-силния удар в този кадър (m/s), 0 ако няма удар.
    float collide(Car& car);

    // lampsOn - светят ли лампите; night - 0 (ден) .. 1 (нощ) за прозорците
    void draw(const Shader& shader, bool lampsOn, float night) const;

    const std::vector<PointLight>& lampLights() const { return m_lampLights; }
    int treeCount() const { return static_cast<int>(m_pineCount + m_oakCount); }

private:
    // ---- Описания на обектите ----
    struct Building {
        glm::mat4 frame;          // локални оси: X - по пътя, Y - нагоре, Z - встрани
        glm::vec3 size;           // дължина, височина, ширина
        float     baseY;          // колко под нивото на земята започва
        glm::vec3 wallColor;
        OBB       box;
    };
    struct RailSegment {
        glm::mat4 model;          // самата греда
        glm::mat4 post;           // стълбче в началото на сегмента
        OBB       box;
    };
    struct Cone {
        glm::vec3 home;           // начална позиция (за reset)
        glm::vec3 pos;
        glm::vec2 velocity{0.0f}; // след удар се плъзга
        glm::vec2 fallDir {1.0f, 0.0f};
        float     tilt = 0.0f;    // 0 = изправен, 90 = легнал (градуси)
        bool      knocked = false;
    };
    struct Prop {                 // произволен статичен обект от една част
        const Mesh* mesh;
        glm::mat4   model;
        Material    material;
    };

    void createTrees(const Road& road, const Terrain& terrain);
    void createBuildings(const Road& road, const Terrain& terrain);
    void createGuardrails(const Road& road, const Terrain& terrain);
    void createLampsAndProps(const Road& road, const Terrain& terrain);
    void createCones(const Road& road, const Terrain& terrain);

    // ---- Геометрия ----
    std::unique_ptr<Mesh> m_cube, m_cylinder, m_sphere, m_cone;
    // Отделни mesh-ове за дърветата - всеки има собствен буфер с копия
    std::unique_ptr<Mesh> m_pineTrunk, m_pineCrown, m_oakTrunk, m_oakCrown;
    size_t m_pineCount = 0, m_oakCount = 0;

    // ---- Обекти ----
    std::vector<Building>    m_buildings;
    std::vector<RailSegment> m_rails;
    std::vector<Cone>        m_cones;
    std::vector<Prop>        m_props;          // бали, сфери
    std::vector<glm::mat4>   m_lamps;          // матрица на всяка лампа
    std::vector<PointLight>  m_lampLights;

    // ---- Форми за колизии на статичните обекти ----
    std::vector<Circle> m_staticCircles;       // дървета, стълбове, бали, сфери
    std::vector<OBB>    m_staticBoxes;         // сгради, мантинели

    const Terrain* m_terrain;
};
