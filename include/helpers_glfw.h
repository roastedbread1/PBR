//#pragma once
//#include <stdio.h>
//
//#define GLFW_INCLUDE_NONE
//#include <GLFW/glfw3.h>
//
//#include <GLFW/glfw3native.h>
//
//inline GLFWwindow* init_window(const char* title, uint32_t& width, uint32_t& height)
//{
//    glfwSetErrorCallback([](int error, const char* description) { printf("GLFW Error (%i): %s\n", error, description); });
//
//    if (!glfwInit()) {
//        return nullptr;
//    }
//
//    const bool wantsWholeArea = !width || !height;
//
//    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//    glfwWindowHint(GLFW_RESIZABLE, wantsWholeArea ? GLFW_FALSE : GLFW_TRUE);
//
//    // render full screen without overlapping taskbar
//    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
//    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
//
//    int x = 0;
//    int y = 0;
//    int w = mode->width;
//    int h = mode->height;
//
//    if (wantsWholeArea) {
//        glfwGetMonitorWorkarea(monitor, &x, &y, &w, &h);
//    }
//    else {
//        w = width;
//        h = height;
//    }
//
//    GLFWwindow* window = glfwCreateWindow(w, h, title, nullptr, nullptr);
//
//    if (!window) {
//        glfwTerminate();
//        return nullptr;
//    }
//
//    if (wantsWholeArea) {
//        glfwSetWindowPos(window, x, y);
//    }
//
//    glfwGetWindowSize(window, &w, &h);
//
//    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int, int action, int) {
//        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
//            glfwSetWindowShouldClose(window, GLFW_TRUE);
//        }
//        });
//
//    width = (uint32_t)w;
//    height = (uint32_t)h;
//
//    return window;
//};