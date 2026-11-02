#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

Camera::Camera(const glm::vec3& position, float yaw, float pitch)
    : m_position(position), m_yaw(yaw), m_pitch(pitch) {
    updateVectors();
}

// ------------------------------------------------------------
//  View матрица: преобразува света в координатна система,
//  в която камерата е в началото и гледа по -Z.
//  lookAt(око, точка_към_която_гледаме, посока_нагоре)
// ------------------------------------------------------------
glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

// ------------------------------------------------------------
//  Перспективна проекция: далечните обекти изглеждат по-малки.
//  Определя се от зрителния ъгъл (FOV), съотношението
//  ширина/височина и близката/далечната отсичаща равнина.
// ------------------------------------------------------------
glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(m_fov), aspectRatio, nearPlane, farPlane);
}

// ------------------------------------------------------------
//  Движение с клавиатурата. Изминатото разстояние е
//  скорост * deltaTime, т.е. движението е еднакво бързо
//  независимо от броя кадри в секунда (FPS).
// ------------------------------------------------------------
void Camera::processKeyboard(CameraMove dir, float deltaTime, bool fast) {
    float velocity = moveSpeed * deltaTime * (fast ? 3.0f : 1.0f);
    switch (dir) {
        case CameraMove::Forward:  m_position += m_front   * velocity; break;
        case CameraMove::Backward: m_position -= m_front   * velocity; break;
        case CameraMove::Left:     m_position -= m_right   * velocity; break;
        case CameraMove::Right:    m_position += m_right   * velocity; break;
        case CameraMove::Up:       m_position += m_worldUp * velocity; break;
        case CameraMove::Down:     m_position -= m_worldUp * velocity; break;
    }
}

// ------------------------------------------------------------
//  Мишката променя ъглите yaw и pitch.
//  pitch се ограничава до ±89°, за да не се "преобърне"
//  камерата (при 90° front става успореден на worldUp и
//  векторното произведение в updateVectors() става нула).
// ------------------------------------------------------------
void Camera::processMouseMovement(float xOffset, float yOffset) {
    m_yaw   += xOffset * mouseSensitivity;
    m_pitch += yOffset * mouseSensitivity;
    m_pitch  = std::clamp(m_pitch, -89.0f, 89.0f);
    updateVectors();
}

// Колелцето на мишката променя FOV -> ефект на приближаване (zoom)
void Camera::processMouseScroll(float yOffset) {
    m_fov = std::clamp(m_fov - yOffset * 2.0f, 20.0f, 90.0f);
}

// ------------------------------------------------------------
//  Преобразуване от сферични координати (yaw, pitch) към
//  единичен вектор на посоката:
//      x = cos(yaw) * cos(pitch)
//      y = sin(pitch)
//      z = sin(yaw) * cos(pitch)
//  right и up се получават чрез векторни произведения, така
//  трите вектора образуват ортонормиран базис.
// ------------------------------------------------------------
void Camera::updateVectors() {
    float yawR   = glm::radians(m_yaw);
    float pitchR = glm::radians(m_pitch);

    glm::vec3 f;
    f.x = std::cos(yawR) * std::cos(pitchR);
    f.y = std::sin(pitchR);
    f.z = std::sin(yawR) * std::cos(pitchR);

    m_front = glm::normalize(f);
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up    = glm::normalize(glm::cross(m_right, m_front));
}
