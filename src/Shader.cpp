#include "Shader.h"

#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

static std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Не може да се отвори файл: " + path);
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint Shader::compile(GLenum type, const std::string& source, const std::string& path) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        throw std::runtime_error("Грешка при компилиране на " + path + ":\n" + log);
    }
    return shader;
}

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
    GLuint vs = compile(GL_VERTEX_SHADER,   readFile(vertexPath),   vertexPath);
    GLuint fs = compile(GL_FRAGMENT_SHADER, readFile(fragmentPath), fragmentPath);

    m_program = glCreateProgram();
    glAttachShader(m_program, vs);
    glAttachShader(m_program, fs);
    glLinkProgram(m_program);

    GLint ok = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(m_program, sizeof(log), nullptr, log);
        throw std::runtime_error(std::string("Грешка при свързване на шейдъри:\n") + log);
    }
    // След свързването отделните шейдъри вече не са нужни
    glDeleteShader(vs);
    glDeleteShader(fs);
}

Shader::~Shader() { glDeleteProgram(m_program); }

void Shader::use() const { glUseProgram(m_program); }

void Shader::setInt(const char* n, int v) const {
    glUniform1i(glGetUniformLocation(m_program, n), v);
}
void Shader::setFloat(const char* n, float v) const {
    glUniform1f(glGetUniformLocation(m_program, n), v);
}
void Shader::setVec3(const char* n, const glm::vec3& v) const {
    glUniform3fv(glGetUniformLocation(m_program, n), 1, glm::value_ptr(v));
}
void Shader::setMat3(const char* n, const glm::mat3& m) const {
    glUniformMatrix3fv(glGetUniformLocation(m_program, n), 1, GL_FALSE, glm::value_ptr(m));
}
void Shader::setMat4(const char* n, const glm::mat4& m) const {
    glUniformMatrix4fv(glGetUniformLocation(m_program, n), 1, GL_FALSE, glm::value_ptr(m));
}
