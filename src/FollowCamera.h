#pragma once
// ============================================================
//  FollowCamera - камера, която следва автомобила.
//
//  Режими:
//    ThirdPerson - от трето лице: зад и над колата. Посоката ѝ
//                  "догонва" посоката на колата със закъснение,
//                  затова в завой виждаме колата леко отстрани.
//    Hood        - от първо лице: над капака, гледа напред и се
//                  накланя заедно с колата.
//
//  В режим ThirdPerson позицията на камерата се задава в сферични
//  координати около точка над колата (pivot):
//      разстояние (distance), азимут (yaw), елевация (pitch)
//  Мишката временно завърта камерата около колата (оглеждане),
//  след което тя плавно се връща зад колата.
// ============================================================
#include <glm/glm.hpp>

class Car;
class Terrain;

enum class FollowMode { ThirdPerson, Hood };

class FollowCamera {
public:
    // Мигновено поставяне зад колата (при старт и при reset)
    void snapTo(const Car& car);

    void update(const Car& car, const Terrain& terrain, float dt);

    // Вход от мишката: оглеждане около колата (в градуси)
    void addOrbit(float dYawDeg, float dPitchDeg);
    // Колелце на мишката: приближаване / отдалечаване
    void zoom(float scroll);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

    const glm::vec3& position() const { return m_position; }
    const glm::vec3& target()   const { return m_target; }
    float fov() const { return m_fov; }

    FollowMode mode = FollowMode::ThirdPerson;

private:
    // Позиция и ориентация, от които се строи View матрицата
    glm::vec3 m_position{0.0f};
    glm::vec3 m_target  {0.0f, 0.0f, -1.0f};
    glm::vec3 m_up      {0.0f, 1.0f, 0.0f};

    // Изгладени (закъсняващи) стойности
    float m_yaw  = 0.0f;      // посока на камерата (rad) - "гони" посоката на колата
    float m_carY = 0.0f;      // височина на колата - изгладена срещу подрусване
    float m_fov  = 60.0f;     // градуси - расте със скоростта

    // Оглеждане с мишката (градуси) - отслабва с времето
    float m_orbitYaw   = 0.0f;
    float m_orbitPitch = 0.0f;

    float m_distance = 7.5f;  // метри зад колата
};
