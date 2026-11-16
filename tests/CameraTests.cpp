#include "engine/Renderer/Camera.h"

#include <glm/glm.hpp>

#include "TestFramework.h"

namespace
{
void ExpectUnitVector(const glm::vec3& vector)
{
    CHECK_NEAR(glm::length(vector), 1.0f, 1e-5f);
}

void ExpectOrthogonal(const glm::vec3& a, const glm::vec3& b)
{
    CHECK_NEAR(glm::dot(a, b), 0.0f, 1e-5f);
}
} // namespace

TEST(DefaultCameraState)
{
    Camera camera;

    CHECK_NEAR(camera.GetPosition().x, 0.0f, 1e-6f);
    CHECK_NEAR(camera.GetPosition().y, 0.0f, 1e-6f);
    CHECK_NEAR(camera.GetPosition().z, 3.0f, 1e-6f);

    CHECK_NEAR(camera.GetFront().x, 0.0f, 1e-6f);
    CHECK_NEAR(camera.GetFront().y, 0.0f, 1e-6f);
    CHECK_NEAR(camera.GetFront().z, -1.0f, 1e-6f);

    CHECK_NEAR(camera.GetZoom(), 45.0f, 1e-6f);
    ExpectUnitVector(camera.GetFront());
}

TEST(ViewMatrixMapsCameraToOrigin)
{
    Camera camera(glm::vec3(1.0f, 2.0f, 3.0f), -90.0f, 12.0f);

    const glm::vec4 origin = camera.GetViewMatrix() * glm::vec4(camera.GetPosition(), 1.0f);

    CHECK_NEAR(origin.x, 0.0f, 1e-5f);
    CHECK_NEAR(origin.y, 0.0f, 1e-5f);
    CHECK_NEAR(origin.z, 0.0f, 1e-5f);
    CHECK_NEAR(origin.w, 1.0f, 1e-5f);
}

TEST(ProjectionMatrixHonoursAspectRatio)
{
    Camera camera;
    const float aspect = 16.0f / 9.0f;

    const glm::mat4 projection = camera.GetProjectionMatrix(aspect);

    CHECK_NEAR(projection[3][3], 0.0f, 1e-6f);
    CHECK_NEAR(projection[1][1] / projection[0][0], aspect, 1e-5f);
}

TEST(KeyboardMovesAlongLocalAxes)
{
    Camera camera(glm::vec3(0.0f), -90.0f, 0.0f);
    camera.SetMovementSpeed(2.0f);

    camera.ProcessKeyboard(0.5f, CameraMovement::Forward);
    CHECK_NEAR(camera.GetPosition().z, -1.0f, 1e-5f);

    camera.ProcessKeyboard(0.5f, CameraMovement::Backward);
    CHECK_NEAR(camera.GetPosition().z, 0.0f, 1e-5f);

    camera.ProcessKeyboard(0.5f, CameraMovement::Right);
    CHECK_NEAR(camera.GetPosition().x, 1.0f, 1e-5f);

    camera.ProcessKeyboard(0.5f, CameraMovement::Left);
    CHECK_NEAR(camera.GetPosition().x, 0.0f, 1e-5f);

    camera.ProcessKeyboard(0.5f, CameraMovement::Up);
    CHECK_NEAR(camera.GetPosition().y, 1.0f, 1e-5f);

    camera.ProcessKeyboard(0.5f, CameraMovement::Down);
    CHECK_NEAR(camera.GetPosition().y, 0.0f, 1e-5f);
}

TEST(MovementIsFrameRateIndependent)
{
    Camera a(glm::vec3(0.0f), -90.0f, 0.0f);
    Camera b(glm::vec3(0.0f), -90.0f, 0.0f);
    a.SetMovementSpeed(3.0f);
    b.SetMovementSpeed(3.0f);

    for (int i = 0; i < 60; ++i)
    {
        a.ProcessKeyboard(1.0f / 60.0f, CameraMovement::Forward);
    }
    b.ProcessKeyboard(1.0f, CameraMovement::Forward);

    CHECK_NEAR(a.GetPosition().z, b.GetPosition().z, 1e-5f);
}

TEST(MouseChangesYawAndClampsPitch)
{
    Camera camera(glm::vec3(0.0f), -90.0f, 0.0f);
    camera.SetMouseSensitivity(0.1f);

    camera.ProcessMouse(10.0f, 0.0f);
    CHECK_NEAR(camera.GetFront().x, std::sin(glm::radians(1.0f)), 1e-4f);

    camera.ProcessMouse(0.0f, 100000.0f);
    CHECK_NEAR(camera.GetFront().y, std::sin(glm::radians(89.0f)), 1e-4f);

    camera.ProcessMouse(0.0f, -1000000.0f);
    CHECK_NEAR(camera.GetFront().y, std::sin(glm::radians(-89.0f)), 1e-4f);
}

TEST(BasisStaysOrthonormalAfterRotation)
{
    Camera camera(glm::vec3(0.0f), 33.0f, -21.0f);
    camera.ProcessMouse(25.0f, 17.0f);

    const glm::vec3 front = camera.GetFront();
    const glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));

    ExpectUnitVector(front);
    ExpectUnitVector(right);
    ExpectOrthogonal(front, right);
}

TEST(ScrollClampsZoomToSupportedRange)
{
    Camera camera;

    camera.ProcessMouseScroll(1.0f);
    CHECK_NEAR(camera.GetZoom(), 44.0f, 1e-6f);

    camera.ProcessMouseScroll(1000.0f);
    CHECK_NEAR(camera.GetZoom(), 1.0f, 1e-6f);

    camera.ProcessMouseScroll(-1000.0f);
    CHECK_NEAR(camera.GetZoom(), 45.0f, 1e-6f);
}

int main()
{
    return ::testing::RunAll();
}
