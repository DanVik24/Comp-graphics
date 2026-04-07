#ifndef MODEL_H
#define MODEL_H

#include <GL/GL.h>
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "Mesh.h"
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <memory>

class Model {
public:
    struct MeshEntry {
        Mesh mesh;
        std::string name;

        // Конструктор, принимающий Mesh и имя
        MeshEntry(Mesh&& m, std::string n)
            : mesh(std::move(m)), name(std::move(n)) {
        }
    };

    std::vector<MeshEntry> meshes;

    Model(std::string const& path) {
        loadModel(path);
    }

    void Draw(unsigned int shaderProgram, const std::map<std::string, glm::mat4>& partMatrices) {
        glUseProgram(shaderProgram);
        for (auto& entry : meshes) {
            auto it = partMatrices.find(entry.name);
            glm::mat4 modelMat = (it != partMatrices.end()) ? it->second : glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelMat));
            entry.mesh.Draw();
        }
    }

private:
    void loadModel(std::string const& path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }
        processNode(scene->mRootNode, scene);
    }

    void processNode(aiNode* node, const aiScene* scene) {
        for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            std::string name;
            if (node->mName.length > 0)
                name = std::string(node->mName.C_Str());
            else if (mesh->mName.length > 0)
                name = std::string(mesh->mName.C_Str());
            else
                name = "unnamed_" + std::to_string(i);

            // Создаём MeshEntry с перемещением результата processMesh
            meshes.emplace_back(processMesh(mesh, scene), std::move(name));
        }
        for (unsigned int i = 0; i < node->mNumChildren; ++i) {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            Vertex vertex;
            vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            if (mesh->HasNormals())
                vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            else
                vertex.Normal = glm::vec3(0.0f);
            vertices.push_back(vertex);
        }
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; ++j)
                indices.push_back(face.mIndices[j]);
        }
        return Mesh(std::move(vertices), std::move(indices));
    }
};

#endif