#pragma once

#include <functional>
#include <string>

struct GLFWwindow;

// RAII wrapper around a GLFW window and its OpenGL 3.3 core context.
class Window
{
public:
    // msaaSamples <= 0 disables multisampling.
    Window(int width, int height, const std::string& title, bool vsync = true, int msaaSamples = 4);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;

    bool ShouldClose() const;
    void SwapBuffers();
    void PollEvents();

    using FramebufferSizeCallback = std::function<void(int width, int height)>;
    using KeyCallback = std::function<void(int key, int scancode, int action, int mods)>;
    using CursorPosCallback = std::function<void(double xPos, double yPos)>;
    using ScrollCallback = std::function<void(double xOffset, double yOffset)>;

    void SetFramebufferSizeCallback(FramebufferSizeCallback callback);
    void SetKeyCallback(KeyCallback callback);
    void SetCursorPosCallback(CursorPosCallback callback);
    void SetScrollCallback(ScrollCallback callback);

    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    int GetMSAASamples() const { return m_MSAASamples; }
    GLFWwindow* GetNativeHandle() const { return m_Window; }

private:
    static void FramebufferSizeCallbackBridge(GLFWwindow* window, int width, int height);
    static void KeyCallbackBridge(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void CursorPosCallbackBridge(GLFWwindow* window, double xPos, double yPos);
    static void ScrollCallbackBridge(GLFWwindow* window, double xOffset, double yOffset);

    void Destroy();

    GLFWwindow* m_Window = nullptr;
    int m_Width = 0;
    int m_Height = 0;
    int m_MSAASamples = 0;

    FramebufferSizeCallback m_FramebufferSizeCallback;
    KeyCallback m_KeyCallback;
    CursorPosCallback m_CursorPosCallback;
    ScrollCallback m_ScrollCallback;
};
