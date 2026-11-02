#pragma once
// ============================================================
//  Mesh - геометрия, качена във видеопаметта (GPU).
//
//  Всеки връх (Vertex) съдържа:
//    position - позиция в локалните координати на обекта
//    normal   - нормала (нужна за осветлението)
//    uv       - текстурни координати (за текстурирането)
//
//  OpenGL обекти:
//    VBO (Vertex Buffer Object)  - масивът с върховете
//    EBO (Element Buffer Object) - индексите на триъгълниците
//    VAO (Vertex Array Object)   - "рецептата" как се четат
//                                  данните от VBO към шейдъра
// ============================================================
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void draw() const;

    // ---- Инстанциране (instancing) ----
    // Качва в GPU масив от Model матрици - по една за всяко копие.
    // След това drawInstanced() рисува ВСИЧКИ копия с една заявка.
    void setInstances(const std::vector<glm::mat4>& matrices);
    void drawInstanced() const;

private:
    GLuint m_vao = 0, m_vbo = 0, m_ebo = 0;
    GLsizei m_indexCount = 0;
    GLuint  m_instanceVbo   = 0;
    GLsizei m_instanceCount = 0;
};

// Фабрични функции за основни геометрични примитиви
namespace Primitives {
    Mesh* createCube();                                 // куб 1x1x1 с център (0,0,0)
    Mesh* createPlane(float size, float uvRepeat = 1);  // хоризонтална равнина size x size
    Mesh* createSphere(int segments = 32, int rings = 16); // сфера с диаметър 1
    Mesh* createCylinder(int segments = 24);             // цилиндър: диаметър 1, височина 1, ос Y
    Mesh* createCone(int segments = 24);                 // конус: диаметър на основата 1, височина 1
}
