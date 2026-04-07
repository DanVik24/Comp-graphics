#ifndef MESH_H
#define MESH_H

#include <GL/GL.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <vector>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO;

    // Конструктор с параметрами
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices)
        : vertices(std::move(vertices)), indices(std::move(indices))
    {
        setupMesh();
    }

    // Конструктор перемещения
    Mesh(Mesh&& other) noexcept
        : vertices(std::move(other.vertices)),
        indices(std::move(other.indices)),
        VAO(other.VAO),
        VBO(other.VBO),
        EBO(other.EBO)
    {
        other.VAO = other.VBO = other.EBO = 0;
    }

    // Запрещаем копирование
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Оператор перемещения
    Mesh& operator=(Mesh&& other) noexcept {
        if (this != &other) {
            vertices = std::move(other.vertices);
            indices = std::move(other.indices);
            VAO = other.VAO;
            VBO = other.VBO;
            EBO = other.EBO;
            other.VAO = other.VBO = other.EBO = 0;
        }
        return *this;
    }

    ~Mesh() {
        // Опционально: удалить буферы, но необязательно для простоты
    }

    void Draw() const {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

private:
    unsigned int VBO, EBO;

    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

        glBindVertexArray(0);
    }
};

#endif