#pragma once

#include <glm/glm.hpp>

enum class CameraMovement
{
    Forward,
    Backward,
    Left,
    Right,
    Up,
    Down,
};

// Perspective fly camera driven by yaw/pitch Euler angles.
class Camera
{
public:
    explicit Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
                    float yaw = -90.0f,
                    float pitch = 0.0f);

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspect) const;

    // deltaSeconds is used so movement speed is frame-rate independent.
    void ProcessKeyboard(float deltaSeconds, CameraMovement direction);
    void ProcessMouse(float deltaX, float deltaY);
    void ProcessMouseScroll(float yOffset);

    const glm::vec3& GetPosition() const { return m_Position; }
    const glm::vec3& GetFront() const { return m_Front; }
    float GetZoom() const { return m_Zoom; }

    void SetMovementSpeed(float speed) { m_MovementSpeed = speed; }
    void SetMouseSensitivity(float sensitivity) { m_MouseSensitivity = sensitivity; }

private:
    void UpdateCameraVectors();

    glm::vec3 m_Position;
    glm::vec3 m_Front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 m_Up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 m_Right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 m_WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    float m_Yaw;
    float m_Pitch;

    float m_MovementSpeed = 2.5f;
    float m_MouseSensitivity = 0.1f;
    float m_Zoom = 45.0f;
    float m_Near = 0.1f;
    float m_Far = 100.0f;
};
