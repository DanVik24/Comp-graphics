// подключаем бибилиотеки
#define GLEW_DLL
#define GLFW_DLL

#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <sstream>
#include <string>
#include "glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

// Размер окна
const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;

// Камера
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
float fov = 45.0f;
float yaw = -90.0f;
float pitch = 0.0f;
bool firstMouse = true;
float sensitivity = 0.1f;

float deltaTime= 0.0f;
float lastFrame = 0.0f;

// Прототипы функций
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
std::string readShaderFile(const std::string& filePath);
unsigned int compileShader(GLenum type, const std::string& source);
unsigned int createShaderProgram(const std::string& vertexSource, const std::string& fragmentSource);

int main() {
    // ============ 1. Инициализация GLFW ============
    if (!glfwInit()) {
        std::cerr << "Ошибка инициализации GLFW" << std::endl;
        return -1;
    }

    // Настройка версии OpenGL 4.6 (Core Profile)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // ============ 2. Создание окна ============
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Лабораторная работа №4: Камера и матрицы", NULL, NULL);
    if (!window) {
        std::cerr << "Ошибка создания окна GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    // Скрываем курсор и фиксируем его в центре окна (режим FPS-камеры)
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glewExperimental = GL_TRUE;

    GLenum ret = glewInit();
    if (ret != GLEW_OK) {
        fprintf(stderr, "ERROR: %s\n", glewGetErrorString(ret));
        return 1;
    }

    // ============ Настройка вершин пятиугольника ============
    // Координаты (x, y, z) + цвет (r, g, b)
    float vertices[] = {
        // Вершины
         0.0f,  0.5f,  0.0f,
         0.47f, 0.15f, 0.0f,
         0.29f, -0.4f, 0.0f,
        -0.29f, -0.4f, 0.0f,
        -0.47f, 0.15f, 0.0f,
    };

    unsigned int indices[] = {
        0, 1, 2,  // треугольник 1
        0, 2, 3,  // треугольник 2
        0, 3, 4   // треугольник 3
    };

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Включаем тест глубины для корректного отображения
    glEnable(GL_DEPTH_TEST);


    // ============ Загрузка и компиляция шейдеров ============
    std::string vertexShaderSource = readShaderFile("vertex_shader.glsl");
    std::string fragmentShaderSource = readShaderFile("fragment_shader.glsl");

    if (vertexShaderSource.empty() || fragmentShaderSource.empty()) {
        std::cerr << "ОШИБКА: не удалось загрузить файлы шейдеров!" << std::endl;
        return -1;
    }

    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    if (shaderProgram == 0) return -1;

    // ============ 6. Главный цикл ============
    while (!glfwWindowShouldClose(window)) {
        // Вычисление времени между кадрами
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime= currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Обработка ввода (движение камеры WASD)
        processInput(window);

        // Очистка экрана
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Использование шейдерной программы
        glUseProgram(shaderProgram);

        // Матрица модели (вращение фигуры)
        glm::mat4 model = glm::mat4(1.0f);
        //model = glm::rotate(model, currentFrame * glm::radians(50.0f), glm::vec3(0.5f, 1.0f, 0.0f));

        // Матрица вида (камера)
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        // Матрица проекции (перспектива)
        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),                 // угол обзора
            (float)SCR_WIDTH / (float)SCR_HEIGHT, // соотношение сторон
            0.1f,                                 // ближняя плоскость отсечения
            100.0f                                // дальняя плоскость отсечения
        );

        // Передача матриц в шейдер
        unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
        unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
        unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Отрисовка пятиугольника (9 индексов, 3 треугольника)
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 9, GL_UNSIGNED_INT, 0);

        // Обмен буферов и обработка событий
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ============ 7. Освобождение ресурсов ============
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();

    return 0;
}

// ============ Реализация вспомогательных функций ============

// Изменение размера окна
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Обработка ввода с клавиатуры (движение камеры)
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = 2.5f * deltaTime; // скорость движения (единиц в секунду)

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

// Обработка движения мыши (поворот камеры)
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;


    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

// Чтение текста шейдера из файла
std::string readShaderFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "ОШИБКА: не удалось открыть файл шейдера: " << filePath << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}

// Компиляция шейдера (вершинного или фрагментного)
unsigned int compileShader(GLenum type, const std::string& source) {
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);


    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ОШИБКА компиляции шейдера:\n" << infoLog << std::endl;
        return 0;
    }
    return shader;
}

// Создание шейдерной программы из двух скомпилированных шейдеров
unsigned int createShaderProgram(const std::string& vertexSource, const std::string& fragmentSource) {
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    if (vertexShader == 0 || fragmentShader == 0) return 0;

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "ОШИБКА линковки шейдерной программы:\n" << infoLog << std::endl;
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}