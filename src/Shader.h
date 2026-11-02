#pragma once
// Shader - зарежда, компилира и свързва vertex + fragment шейдър
// и предоставя удобни методи за задаване на uniform променливи.

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class Shader {
public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;

    void setInt  (const char* name, int v) const;
    void setFloat(const char* name, float v) const;
    void setVec3 (const char* name, const glm::vec3& v) const;
    void setMat3 (const char* name, const glm::mat3& m) const;
    void setMat4 (const char* name, const glm::mat4& m) const;

    GLuint id() const { return m_program; }

private:
    GLuint m_program = 0;
    static GLuint compile(GLenum type, const std::string& source, const std::string& path);
};
