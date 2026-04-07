#ifndef GRAFSHADERS_H
#define GRAFSHADERS_H

#include <string>

class Shader {
public:
    unsigned int shaderProgram;   // ID шейдерной программы (публичный, как используетс€ в коде)

    Shader();
    int load(const char* vert_sh_path, const char* frag_sh_path);
    void use();

    // ћетоды дл€ установки uniform-цветов (используютс€ в некоторых верси€х, но не об€зательны)
    void glUniform(const char* cl_name, float r, float g, float b);
    void glUniform(const char* cl_name, float r, float g, float b, float a);
    void glUniform(const char* cl_name, int r, int g, int b);
    void glUniform(const char* cl_name, int r, int g, int b, float a);

private:
    std::string readShFile(const char* path);
    void validate(float& _v, float _min, float _max);
    void validate(int& _v, int _min, int _max);
    float intToFloat(int& _v);
};

#endif