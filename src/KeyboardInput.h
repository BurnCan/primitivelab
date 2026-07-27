#pragma once

#include <GLFW/glfw3.h>

#if defined(_WIN32) && defined(PRIMITIVELAB_BACKGROUND_KEYBOARD_INPUT)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

// GLFW's key state is populated from messages sent to its window. On Windows,
// a newly launched fullscreen process can be visible and have its cursor
// captured before the OS grants it foreground keyboard focus. Polling the
// physical key state avoids making the first mouse click a prerequisite for
// gameplay input. Other platforms retain GLFW's normal per-window behavior.
inline bool IsKeyPressed(GLFWwindow* window, int key)
{
#if defined(_WIN32) && defined(PRIMITIVELAB_BACKGROUND_KEYBOARD_INPUT)
    int virtualKey = 0;
    switch (key) {
        case GLFW_KEY_W:             virtualKey = 'W'; break;
        case GLFW_KEY_A:             virtualKey = 'A'; break;
        case GLFW_KEY_S:             virtualKey = 'S'; break;
        case GLFW_KEY_D:             virtualKey = 'D'; break;
        case GLFW_KEY_SPACE:         virtualKey = VK_SPACE; break;
        case GLFW_KEY_TAB:           virtualKey = VK_TAB; break;
        case GLFW_KEY_F1:            virtualKey = VK_F1; break;
        case GLFW_KEY_LEFT_CONTROL:  virtualKey = VK_LCONTROL; break;
        case GLFW_KEY_RIGHT_CONTROL: virtualKey = VK_RCONTROL; break;
        default: break;
    }

    if (virtualKey != 0)
        return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
#endif

    return glfwGetKey(window, key) == GLFW_PRESS;
}
