#include "engine/Renderer/Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

Camera::Camera(glm::vec3 position, float yaw, float pitch)
    : m_Position(position), m_Yaw(yaw), m_Pitch(pitch)
{
    UpdateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(m_Position, m_Position + m_Front, m_Up);
}

glm::mat4 Camera::GetProjectionMatrix(float aspect) const
{
    return glm::perspective(glm::radians(m_Zoom), aspect, m_Near, m_Far);
}

void Camera::ProcessKeyboard(float deltaSeconds, CameraMovement direction)
{
    const float velocity = m_MovementSpeed * deltaSeconds;

    switch (direction)
    {
    case CameraMovement::Forward:
        m_Position += m_Front * velocity;
        break;
    case CameraMovement::Backward:
        m_Position -= m_Front * velocity;
        break;
    case CameraMovement::Left:
        m_Position -= m_Right * velocity;
        break;
    case CameraMovement::Right:
        m_Position += m_Right * velocity;
        break;
    case CameraMovement::Up:
        m_Position += m_WorldUp * velocity;
        break;
    case CameraMovement::Down:
        m_Position -= m_WorldUp * velocity;
        break;
    }
}

void Camera::ProcessMouse(float deltaX, float deltaY)
{
    m_Yaw += deltaX * m_MouseSensitivity;
    m_Pitch += deltaY * m_MouseSensitivity;
    m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);

    UpdateCameraVectors();
}

void Camera::ProcessMouseScroll(float yOffset)
{
    m_Zoom = std::clamp(m_Zoom - yOffset, 1.0f, 45.0f);
}

void Camera::UpdateCameraVectors()
{
    const float yaw = glm::radians(m_Yaw);
    const float pitch = glm::radians(m_Pitch);

    glm::vec3 front;
    front.x = std::cos(yaw) * std::cos(pitch);
    front.y = std::sin(pitch);
    front.z = std::sin(yaw) * std::cos(pitch);

    m_Front = glm::normalize(front);
    m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
    m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}
