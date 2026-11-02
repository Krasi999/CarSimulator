#pragma once
// ============================================================
//  Car - автомобил: физика на движението + йерархичен 3D модел.
//
//  Физика (опростена, "аркадна", но с реални величини в SI):
//    - надлъжно движение: 2-ри закон на Нютон  a = ΣF / m
//      (двигател, спирачки, въздушно съпротивление, триене при
//       търкаляне, компонента на гравитацията по наклона)
//    - завиване: кинематичен модел на велосипеда (bicycle model)
//    - следване на релефа: колата се накланя според терена
//
//  Локална координатна система на колата:
//    +X - напред,  +Y - нагоре,  +Z - надясно
//    (0,0,0) е на земята, в средата между осите
// ============================================================
#include <glm/glm.hpp>

#include "Collision.h"

class Shader;
class Mesh;
class Terrain;
class Road;

// Команди от водача (от клавиатурата)
struct CarInput {
    float throttle  = 0.0f;   // газ       0..1
    float brake     = 0.0f;   // спирачка  0..1 (при спряла кола - заден ход)
    float steer     = 0.0f;   // волан    -1 (ляво) .. +1 (дясно)
    bool  handbrake = false;  // ръчна спирачка
};

class Car {
public:
    Car(const glm::vec3& position, float headingRadians);

    void update(float dt, const CarInput& input, const Terrain& terrain, const Road& road);
    void draw(const Shader& shader, const Mesh& cube, const Mesh& cylinder,
              const Mesh& plane) const;

    void reset(const glm::vec3& position, float headingRadians);

    // ---- Колизии ----
    // Правоъгълникът на колата, гледан отгоре
    OBB collisionBox() const;
    // Скоростта като 2D вектор (X, Z)
    glm::vec2 velocity2D() const;
    // Реакция на удар: избутване навън + промяна на скоростта.
    // normal - накъде да се избута колата; връща скоростта на удара (m/s)
    float applyCollision(const glm::vec2& normal, float depth, float restitution);
    // Загуба на скорост (напр. при удар в лек обект - конус)
    void scaleSpeed(float factor) { m_speed *= factor; }

    // Матрица "кола -> свят": колони = forward, up, right, позиция
    glm::mat4 modelMatrix() const;

    const glm::vec3& position() const { return m_position; }
    const glm::vec3& forward()  const { return m_forward; }
    const glm::vec3& up()       const { return m_up; }
    float heading()   const { return m_heading; }
    float speed()     const { return m_speed; }           // m/s (отрицателна при заден ход)
    float speedKmh()  const { return m_speed * 3.6f; }
    float steerDeg()  const { return glm::degrees(m_steer); }
    bool  onRoad()    const { return m_onRoad; }

    // ---- Размери (метри) ----
    static constexpr float kWheelBase    = 2.70f;   // разстояние между осите
    static constexpr float kTrackHalf    = 0.88f;   // половин разстояние между колелата
    static constexpr float kWheelRadius  = 0.36f;
    static constexpr float kWheelWidth   = 0.28f;
    static constexpr float kLength       = 4.40f;
    static constexpr float kWidth        = 1.90f;

private:
    void followTerrain(float dt, const Terrain& terrain);

    // ---- Състояние ----
    glm::vec3 m_position;
    float     m_heading;               // ъгъл на посоката в хоризонталната равнина (rad)
    float     m_speed     = 0.0f;      // скорост по посоката на колата (m/s)
    float     m_steer     = 0.0f;      // текущ ъгъл на предните колела (rad)
    float     m_wheelSpin = 0.0f;      // натрупан ъгъл на въртене на колелата (rad)
    bool      m_onRoad    = true;
    bool      m_braking   = false;     // за стоп-светлините

    glm::vec3 m_forward{1.0f, 0.0f, 0.0f};
    glm::vec3 m_up     {0.0f, 1.0f, 0.0f};
    glm::vec3 m_right  {0.0f, 0.0f, 1.0f};
};
