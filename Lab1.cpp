// Lab1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#define GLEW_DLL
#define GLFW_DLL
#include "GL/glew.h"
#include "GLFW/glfw3.h"

int main()
{
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW3. \n");
        return 1;
    };

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow* window = glfwCreateWindow(512, 512, "Mainwindow", NULL, NULL);

    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;

    GLenum ret = glewInit();
    if (GLEW_OK != ret) {
        fprintf(stderr, "Error^ %s\n", glewGetErrorString(ret));
        return 1;
    }
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.8f, 0.2f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glColor3f(0.3f, 0.6f, 0.1f);

        glBegin(GL_POLYGON);
            glVertex2f(0.0f, 0.5f);   
            glVertex2f(0.47f, 0.15f);  
            glVertex2f(0.29f, -0.4f);   
            glVertex2f(-0.29f, -0.4f);   
            glVertex2f(-0.47f, 0.15f);  
        glEnd();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}