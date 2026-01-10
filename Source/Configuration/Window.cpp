//
// Created by aspadien on 06.01.2026.
//
#include "../../Headers/Render/Window.hpp"
#include <stdexcept> // Для std::runtime_error
#include <iostream>

// ==========================================
// КОНСТРУКТОР (Теперь чистый и понятный)
// ==========================================
Window::Window(Camera& camera, const char* windowName) {
    // 1. Поднимаем GLFW
    initGLFW();

    // 2. Задаем настройки перед созданием
    configureWindowHints();

    // 3. Создаем само окно
    createWindowObject(windowName);

    // 4. Привязываем данные игрока (Камеру) к окну
    glfwSetWindowUserPointer(window, &camera);
    glfwSetKeyCallback(window, key_callback);

    // 5. Делаем контекст текущим
    glfwMakeContextCurrent(window);

    // 6. Грузим функции OpenGL
    initGLAD();

    // 7. Настраиваем глобальные параметры рендера
    configureOpenGLState();
}

// ==========================================
// ДЕТАЛИ РЕАЛИЗАЦИИ (Private Methods)
// ==========================================

void Window::initGLFW() {
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        // Лучше выбросить исключение, чем жесткий exit(),
        // так сработают деструкторы других объектов
        throw std::runtime_error("Failed to initialize GLFW");
    }
}

void Window::configureWindowHints() {
    // Версия OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Буферы
    glfwWindowHint(GLFW_SAMPLES, 0); // MSAA выключен (или управляется вручную)
    glfwWindowHint(GLFW_DEPTH_BITS, 32); // Высокая точность глубины
    glfwWindowHint(GLFW_CONTEXT_RELEASE_BEHAVIOR, GLFW_RELEASE_BEHAVIOR_NONE);

    // Debug контекст
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    // Настройки окна (лучше задавать ДО создания через Hints, а не после через SetAttrib)
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);     // Окно поверх всех
    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE); // Не сворачивать при потере фокуса (для fullscreen)
}

void Window::createWindowObject(const char* title) {
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);

    // Создаем окно на весь размер экрана, но в оконном режиме (nullptr вместо монитора)
    window = glfwCreateWindow(mode->width, mode->height, title, nullptr, nullptr);
    // = glfwCreateWindow(mode->width, mode->height, title, primary, nullptr);

    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    // Растягиваем на весь экран
    glfwMaximizeWindow(window);
}

void Window::initGLAD() {
    // Правильная загрузка GLAD с использованием загрузчика GLFW
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLAD");
    }
}

void Window::configureOpenGLState() {
    // 1. Глубина и отсечение граней
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // 2. Настройка алгоритма отсечения
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glProvokingVertex(GL_LAST_VERTEX_CONVENTION);

    // 3. Reversed Z-Buffer (Отличная вещь для больших дистанций!)
    // Требует OpenGL 4.5+
    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    glDepthFunc(GL_GREATER);
    glClearDepth(0.0); // При Reversed-Z дальняя плоскость = 0, ближняя = 1

    // 4. Прочее
    glEnable(GL_MULTISAMPLE); // Работает только если буфер создан с сэмплами
    glClearColor(0.4f, 0.4f, 0.8f, 1.0f);
}