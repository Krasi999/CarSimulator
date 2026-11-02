#pragma once
// ============================================================
//  Camera - свободна камера тип "FPS / fly camera".
//
//  Ориентацията се задава с два ъгъла на Ойлер:
//    yaw   - завъртане наляво/надясно (около оста Y)
//    pitch - накланяне нагоре/надолу
//  От тях се изчисляват три взаимно перпендикулярни вектора:
//    front - накъде гледа камерата
//    right - надясно от камерата
//    up    - нагоре спрямо камерата
//  Матрицата на изгледа (View) се строи с glm::lookAt,
//  а проекционната - с glm::perspective.
// ============================================================
#include <glm/glm.hpp>

enum class CameraMove { Forward, Backward, Left, Right, Up, Down };

class Camera {
public:
    explicit Camera(const glm::vec3& position = glm::vec3(0.0f, 2.0f, 8.0f),
                    float yaw = -90.0f, float pitch = -10.0f);

    // --- Матрици ---
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

    // --- Обработка на вход ---
    void processKeyboard(CameraMove direction, float deltaTime, bool fast);
    void processMouseMovement(float xOffset, float yOffset);
    void processMouseScroll(float yOffset);

    // --- Достъп ---
    const glm::vec3& position() const { return m_position; }
    const glm::vec3& front()    const { return m_front; }
    float fov() const { return m_fov; }

    // --- Настройки ---
    float moveSpeed        = 8.0f;   // единици (метри) в секунда
    float mouseSensitivity = 0.1f;   // градуса на пиксел
    float nearPlane        = 0.1f;
    float farPlane         = 500.0f;

private:
    void updateVectors();            // преизчислява front/right/up от yaw и pitch

    glm::vec3 m_position;
    glm::vec3 m_front{0.0f, 0.0f, -1.0f};
    glm::vec3 m_right{1.0f, 0.0f, 0.0f};
    glm::vec3 m_up   {0.0f, 1.0f, 0.0f};
    const glm::vec3 m_worldUp{0.0f, 1.0f, 0.0f};

    float m_yaw;
    float m_pitch;
    float m_fov = 60.0f;             // вертикален зрителен ъгъл в градуси
};
