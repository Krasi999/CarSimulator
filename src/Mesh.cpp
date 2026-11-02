#include "Mesh.h"

#include <cmath>
#include <cstddef>   // offsetof

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
    : m_indexCount(static_cast<GLsizei>(indices.size())) {

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    // 1) Качваме върховете в GPU паметта
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                 vertices.data(), GL_STATIC_DRAW);

    // 2) Качваме индексите
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);

    // 3) Описваме разположението на атрибутите в структурата Vertex.
    //    location-ите съвпадат с layout(location = N) във vertex шейдъра.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

Mesh::~Mesh() {
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
    if (m_instanceVbo) glDeleteBuffers(1, &m_instanceVbo);
}

// ------------------------------------------------------------
//  Инстанциране: допълнителен VBO с по една mat4 на копие.
//  mat4 заема 4 последователни атрибута (по един vec4 за колона):
//  locations 3, 4, 5, 6 във vertex шейдъра.
//  glVertexAttribDivisor(loc, 1) казва: "сменяй стойността веднъж
//  на КОПИЕ, а не за всеки връх".
// ------------------------------------------------------------
void Mesh::setInstances(const std::vector<glm::mat4>& matrices) {
    if (!m_instanceVbo) glGenBuffers(1, &m_instanceVbo);
    m_instanceCount = static_cast<GLsizei>(matrices.size());

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVbo);
    glBufferData(GL_ARRAY_BUFFER, matrices.size() * sizeof(glm::mat4),
                 matrices.data(), GL_STATIC_DRAW);

    for (int col = 0; col < 4; ++col) {
        GLuint loc = 3 + col;
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                              (void*)(sizeof(glm::vec4) * col));
        glVertexAttribDivisor(loc, 1);
    }
    glBindVertexArray(0);
}

void Mesh::drawInstanced() const {
    glBindVertexArray(m_vao);
    glDrawElementsInstanced(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr, m_instanceCount);
    glBindVertexArray(0);
}

void Mesh::draw() const {
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

// ============================================================
//  Примитиви
// ============================================================
namespace Primitives {

// Куб: 6 стени x 4 върха = 24 върха. Върховете НЕ се споделят
// между стените, защото всяка стена има собствена нормала.
Mesh* createCube() {
    struct Face { glm::vec3 n, u, v; };   // нормала и две оси в равнината на стената
    const Face faces[6] = {
        {{ 0, 0, 1}, { 1, 0, 0}, {0, 1, 0}},   // предна   (+Z)
        {{ 0, 0,-1}, {-1, 0, 0}, {0, 1, 0}},   // задна    (-Z)
        {{ 1, 0, 0}, { 0, 0,-1}, {0, 1, 0}},   // дясна    (+X)
        {{-1, 0, 0}, { 0, 0, 1}, {0, 1, 0}},   // лява     (-X)
        {{ 0, 1, 0}, { 1, 0, 0}, {0, 0,-1}},   // горна    (+Y)
        {{ 0,-1, 0}, { 1, 0, 0}, {0, 0, 1}},   // долна    (-Y)
    };

    std::vector<Vertex> verts;
    std::vector<unsigned int> idx;
    for (const Face& f : faces) {
        unsigned int base = static_cast<unsigned int>(verts.size());
        glm::vec3 c = f.n * 0.5f;              // център на стената
        verts.push_back({c - f.u * 0.5f - f.v * 0.5f, f.n, {0, 0}});
        verts.push_back({c + f.u * 0.5f - f.v * 0.5f, f.n, {1, 0}});
        verts.push_back({c + f.u * 0.5f + f.v * 0.5f, f.n, {1, 1}});
        verts.push_back({c - f.u * 0.5f + f.v * 0.5f, f.n, {0, 1}});
        // два триъгълника, обходени обратно на часовниковата стрелка
        idx.insert(idx.end(), {base, base + 1, base + 2, base + 2, base + 3, base});
    }
    return new Mesh(verts, idx);
}

// Хоризонтална равнина в XZ, нормала нагоре (+Y)
Mesh* createPlane(float size, float uvRepeat) {
    float h = size * 0.5f;
    std::vector<Vertex> verts = {
        {{-h, 0,  h}, {0, 1, 0}, {0,        0       }},
        {{ h, 0,  h}, {0, 1, 0}, {uvRepeat, 0       }},
        {{ h, 0, -h}, {0, 1, 0}, {uvRepeat, uvRepeat}},
        {{-h, 0, -h}, {0, 1, 0}, {0,        uvRepeat}},
    };
    std::vector<unsigned int> idx = {0, 1, 2, 2, 3, 0};
    return new Mesh(verts, idx);
}

// ------------------------------------------------------------
//  UV сфера: "паралели" (rings) и "меридиани" (segments), както
//  географските координати. phi - ъгъл от северния полюс (0..PI),
//  theta - ъгъл около оста Y (0..2PI).
//  При сфера с център (0,0,0) нормалата е просто посоката към върха.
// ------------------------------------------------------------
Mesh* createSphere(int segments, int rings) {
    const float PI = 3.14159265358979f;
    std::vector<Vertex> verts;
    std::vector<unsigned int> idx;

    for (int r = 0; r <= rings; ++r) {
        float phi = PI * r / rings;
        for (int s = 0; s <= segments; ++s) {
            float theta = 2.0f * PI * s / segments;
            glm::vec3 n(std::sin(phi) * std::cos(theta),
                        std::cos(phi),
                        std::sin(phi) * std::sin(theta));
            verts.push_back({n * 0.5f, n, {float(s) / segments, 1.0f - float(r) / rings}});
        }
    }
    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < segments; ++s) {
            unsigned int a = r * (segments + 1) + s;   // текущ паралел
            unsigned int b = a + segments + 1;         // следващ (по-долен) паралел
            idx.insert(idx.end(), {a, a + 1, b,  a + 1, b + 1, b});
        }
    }
    return new Mesh(verts, idx);
}

// ------------------------------------------------------------
//  Цилиндър с ос Y: страничната повърхност + два капака.
//  Страната и капаците имат ОТДЕЛНИ върхове, защото нормалите им
//  са различни: настрани (cos, 0, sin) за страната, ±Y за капаците.
// ------------------------------------------------------------
Mesh* createCylinder(int segments) {
    const float PI = 3.14159265358979f;
    std::vector<Vertex> verts;
    std::vector<unsigned int> idx;

    // --- Страна ---
    for (int s = 0; s <= segments; ++s) {
        float a = 2.0f * PI * s / segments;
        glm::vec3 n(std::cos(a), 0.0f, std::sin(a));
        float u = float(s) / segments;
        verts.push_back({{n.x * 0.5f, -0.5f, n.z * 0.5f}, n, {u, 0.0f}});   // долу: 2*s
        verts.push_back({{n.x * 0.5f,  0.5f, n.z * 0.5f}, n, {u, 1.0f}});   // горе: 2*s+1
    }
    for (int s = 0; s < segments; ++s) {
        unsigned int b0 = 2 * s, t0 = b0 + 1, b1 = b0 + 2, t1 = b0 + 3;
        idx.insert(idx.end(), {b0, t0, b1,  t0, t1, b1});
    }

    // --- Капаци (горен: y = +0.5, нормала +Y; долен: y = -0.5, нормала -Y) ---
    for (int cap = 0; cap < 2; ++cap) {
        float y  = cap == 0 ? 0.5f : -0.5f;
        glm::vec3 n(0.0f, cap == 0 ? 1.0f : -1.0f, 0.0f);
        unsigned int center = static_cast<unsigned int>(verts.size());
        verts.push_back({{0.0f, y, 0.0f}, n, {0.5f, 0.5f}});
        for (int s = 0; s <= segments; ++s) {
            float a = 2.0f * PI * s / segments;
            float c = std::cos(a), sn = std::sin(a);
            verts.push_back({{c * 0.5f, y, sn * 0.5f}, n, {0.5f + c * 0.5f, 0.5f + sn * 0.5f}});
        }
        for (int s = 0; s < segments; ++s) {
            unsigned int p0 = center + 1 + s, p1 = p0 + 1;
            if (cap == 0) idx.insert(idx.end(), {center, p1, p0});   // гледано отгоре - CCW
            else          idx.insert(idx.end(), {center, p0, p1});   // гледано отдолу - CCW
        }
    }
    return new Mesh(verts, idx);
}

// ------------------------------------------------------------
//  Конус с ос Y: основа при y = -0.5 (радиус 0.5), връх при y = +0.5.
//  Нормалата на страничната повърхност е наклонена нагоре:
//  за радиус R и височина H тя е (cos a * H, R, sin a * H) -> (cos a, 0.5, sin a).
// ------------------------------------------------------------
Mesh* createCone(int segments) {
    const float PI = 3.14159265358979f;
    std::vector<Vertex> verts;
    std::vector<unsigned int> idx;

    // --- Страна: за всеки сегмент - връх в основата и собствен връх на върха ---
    for (int s = 0; s <= segments; ++s) {
        float a = 2.0f * PI * s / segments;
        glm::vec3 n = glm::normalize(glm::vec3(std::cos(a), 0.5f, std::sin(a)));
        float u = float(s) / segments;
        verts.push_back({{std::cos(a) * 0.5f, -0.5f, std::sin(a) * 0.5f}, n, {u, 0.0f}});
        verts.push_back({{0.0f, 0.5f, 0.0f}, n, {u, 1.0f}});
    }
    for (int s = 0; s < segments; ++s) {
        unsigned int b0 = 2 * s, apex = b0 + 1, b1 = b0 + 2;
        idx.insert(idx.end(), {b0, apex, b1});
    }

    // --- Основа (нормала -Y) ---
    unsigned int center = static_cast<unsigned int>(verts.size());
    verts.push_back({{0.0f, -0.5f, 0.0f}, {0, -1, 0}, {0.5f, 0.5f}});
    for (int s = 0; s <= segments; ++s) {
        float a = 2.0f * PI * s / segments;
        verts.push_back({{std::cos(a) * 0.5f, -0.5f, std::sin(a) * 0.5f}, {0, -1, 0},
                         {0.5f + std::cos(a) * 0.5f, 0.5f + std::sin(a) * 0.5f}});
    }
    for (int s = 0; s < segments; ++s) {
        unsigned int p0 = center + 1 + s;
        idx.insert(idx.end(), {center, p0, p0 + 1});
    }
    return new Mesh(verts, idx);
}

} // namespace Primitives
