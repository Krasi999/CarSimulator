#include "Mesh.h"

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

} // namespace Primitives
