#include "GlfwWindow.h"

#include <iostream>
#include <stdexcept>

#include <QuasarEngine/EngineFactory.h>

#include <GLFW/glfw3.h>

GlfwWindow::GlfwWindow()
    : window(nullptr) {}

GlfwWindow::~GlfwWindow() {
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

void GlfwWindow::Initialize(unsigned int width, unsigned int height, const std::string& title) {
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

void GlfwWindow::SwapBuffers() {
    if (window) {
        glfwSwapBuffers(window);
    }
}

void GlfwWindow::PollEvents() {
    glfwPollEvents();
}

void GlfwWindow::Run()
{

}

void GlfwWindow::Close() {
    if (window) {
        glfwSetWindowShouldClose(window, true);
    }
}

bool GlfwWindow::ShouldClose() const {
    return window && glfwWindowShouldClose(window);
}

void GlfwWindow::Register()
{
    EngineFactory::Instance().RegisterWindow(WindowAPI::GLFW, [] {
        return std::make_unique<GlfwWindow>();
    });
}