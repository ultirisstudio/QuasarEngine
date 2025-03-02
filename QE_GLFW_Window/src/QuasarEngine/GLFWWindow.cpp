#include "GLFWWindow.h"

#include <iostream>
#include <stdexcept>

#include <QuasarEngine/EngineFactory.h>

#include <GLFW/glfw3.h>

GLFWWindow::GLFWWindow(int width, int height, const std::string& title)
    : width(width), height(height), title(title), window(nullptr) {}

GLFWWindow::~GLFWWindow() {
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

void GLFWWindow::Initialize() {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window);
    std::cout << "GLFW Window initialized successfully!" << std::endl;
}

void GLFWWindow::SwapBuffers() {
    if (window) {
        glfwSwapBuffers(window);
    }
}

void GLFWWindow::PollEvents() {
    glfwPollEvents();
}

void GLFWWindow::Run()
{

}

void GLFWWindow::Close() {
    if (window) {
        glfwSetWindowShouldClose(window, true);
    }
}

bool GLFWWindow::ShouldClose() const {
    return window && glfwWindowShouldClose(window);
}

void GLFWWindow::Register()
{
    EngineFactory::Instance().RegisterWindow(WindowAPI::GLFW, [] {
        return std::make_unique<GLFWWindow>(800, 600, "GLFW Window");
    });
}