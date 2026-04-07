#define GLEW_DLL
#define GLFW_DLL

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "Model.h"

const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 1024;

// Камера
glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = 0.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float delta_time = 0.0f;
float last_frame = 0.0f;

// Параметры подвижностей (вариант 7)
float rtkPosX = 0.0f;
float moduleAngle = 0.0f;
float manipulatorZ = 0.0f;

const float RTK_MIN_X = -2.3f;
const float RTK_MAX_X = 2.3f;
const float MODULE_MIN_ANGLE = -60.0f;
const float MODULE_MAX_ANGLE = 60.0f;
const float MANIP_MIN_Z = -0.6f;
const float MANIP_MAX_Z = 0.6f;

// ---------- Функции для работы с шейдерами ----------
std::string readFile(const char* path) {
    std::ifstream file(path, std::ios::in);
    if (!file.is_open()) {
        std::cerr << "Ошибка открытия файла: " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Ошибка компиляции шейдера: " << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint createShaderProgram(const char* vertPath, const char* fragPath) {
    std::string vertSrc = readFile(vertPath);
    std::string fragSrc = readFile(fragPath);
    if (vertSrc.empty() || fragSrc.empty()) return 0;

    GLuint vs = compileShader(GL_VERTEX_SHADER, vertSrc.c_str());
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc.c_str());
    if (!vs || !fs) return 0;

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Ошибка линковки: " << infoLog << std::endl;
        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}
// ----------------------------------------------------

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

static void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }
    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos);
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

static void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = 2.5f * delta_time;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    float speedRTK = 2.0f * delta_time;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        rtkPosX -= speedRTK;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        rtkPosX += speedRTK;
    rtkPosX = glm::clamp(rtkPosX, RTK_MIN_X, RTK_MAX_X);

    float speedAngle = 60.0f * delta_time;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        moduleAngle += speedAngle;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        moduleAngle -= speedAngle;
    moduleAngle = glm::clamp(moduleAngle, MODULE_MIN_ANGLE, MODULE_MAX_ANGLE);

    float speedManip = 2.0f * delta_time;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        manipulatorZ -= speedManip;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        manipulatorZ += speedManip;
    manipulatorZ = glm::clamp(manipulatorZ, MANIP_MIN_Z, MANIP_MAX_Z);
}

int main() {
    if (!glfwInit()) {
        std::cerr << "ERROR: could not start GLFW3.\n";
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
        "RPM-25 Variant 7", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    GLenum ret = glewInit();
    if (ret != GLEW_OK) {
        std::cerr << "GLEW error: " << glewGetErrorString(ret) << std::endl;
        return 1;
    }
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    glEnable(GL_DEPTH_TEST);

    // Загрузка модели
    Model ourModel("3dmodel.obj");
    std::cout << "Модель загружена. Количество мешей: " << ourModel.meshes.size() << std::endl;
    for (const auto& entry : ourModel.meshes) {
        std::cout << "  - Меш: '" << entry.name << "', вершин: " << entry.mesh.vertices.size() << std::endl;
    }

    // Создание шейдерной программы
    GLuint shaderProgram = createShaderProgram("vertex_shader.glsl", "fragment_shader.glsl");
    if (shaderProgram == 0) {
        std::cerr << "Failed to create shader program.\n";
        return 1;
    }
    std::cout << "Шейдерная программа создана, ID: " << shaderProgram << std::endl;

    // Параметры освещения (сделаем ярче для отладки)
    glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
    glm::vec3 lightAmbient(0.3f, 0.3f, 0.3f);
    glm::vec3 lightDiffuse(0.8f, 0.8f, 0.8f);
    glm::vec3 lightSpecular(1.0f, 1.0f, 1.0f);

    glm::vec3 materialAmbient(0.5f, 0.5f, 0.5f);
    glm::vec3 materialDiffuse(0.5f, 0.5f, 0.5f);
    glm::vec3 materialSpecular(0.5f, 0.5f, 0.5f);
    float materialShininess = 32.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        delta_time = currentFrame - last_frame;
        last_frame = currentFrame;

        processInput(window);

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f); // тёмно-серый фон, чтобы видеть даже слабое освещение
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Матрицы вида и проекции
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);

        glUniform3f(glGetUniformLocation(shaderProgram, "material.ambient"), materialAmbient.x, materialAmbient.y, materialAmbient.z);
        glUniform3f(glGetUniformLocation(shaderProgram, "material.diffuse"), materialDiffuse.x, materialDiffuse.y, materialDiffuse.z);
        glUniform3f(glGetUniformLocation(shaderProgram, "material.specular"), materialSpecular.x, materialSpecular.y, materialSpecular.z);
        glUniform1f(glGetUniformLocation(shaderProgram, "material.shininess"), materialShininess);

        glUniform3f(glGetUniformLocation(shaderProgram, "light.position"), lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(glGetUniformLocation(shaderProgram, "light.ambient"), lightAmbient.x, lightAmbient.y, lightAmbient.z);
        glUniform3f(glGetUniformLocation(shaderProgram, "light.diffuse"), lightDiffuse.x, lightDiffuse.y, lightDiffuse.z);
        glUniform3f(glGetUniformLocation(shaderProgram, "light.specular"), lightSpecular.x, lightSpecular.y, lightSpecular.z);

        // Матрицы трансформации
        glm::mat4 identity(1.0f);
        glm::mat4 beamMatrix = identity;
        glm::mat4 rtkMatrix = glm::translate(identity, glm::vec3(rtkPosX, 0.0f, 0.0f));
        glm::mat4 moduleMatrix = glm::rotate(rtkMatrix, glm::radians(moduleAngle), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 manipulatorMatrix = glm::translate(moduleMatrix, glm::vec3(0.0f, 0.0f, manipulatorZ));

        std::map<std::string, glm::mat4> partMatrices;
        partMatrices["Ходовая_балка"] = beamMatrix;
        partMatrices["РТК"] = rtkMatrix;
        partMatrices["модуль"] = moduleMatrix;
        partMatrices["Cube.001"] = manipulatorMatrix;

        // Отладочный вывод текущих матриц (один раз в 100 кадров)
        static int frameCount = 0;
        if (++frameCount % 300 == 0) {
            std::cout << "RTK X: " << rtkPosX << "  Module angle: " << moduleAngle << "  Manip Z: " << manipulatorZ << std::endl;
        }

        ourModel.Draw(shaderProgram, partMatrices);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}