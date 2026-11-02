#include "Renderer.h"
#include "Shader.h"
#include "Mesh.h"
#include "Lighting.h"

#include <glm/gtc/matrix_transform.hpp>

void drawMesh(const Shader& shader, const Mesh& mesh,
              const glm::mat4& model, const Material& material, int mode) {
    shader.setMat4("uModel", model);
    // Нормалната матрица се изчислява ВЕДНЪЖ за обекта тук, на CPU
    shader.setMat3("uNormalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
    uploadMaterial(shader, material);
    shader.setInt("uMode", mode);
    mesh.draw();
}

void drawMeshInstanced(const Shader& shader, const Mesh& mesh,
                       const glm::mat4& partLocal, const Material& material, int mode) {
    shader.setInt("uInstanced", 1);
    shader.setMat4("uModel", partLocal);
    uploadMaterial(shader, material);
    shader.setInt("uMode", mode);
    mesh.drawInstanced();
    shader.setInt("uInstanced", 0);     // връщаме нормалния режим за следващите обекти
}

glm::mat4 partTransform(const glm::vec3& center, const glm::vec3& size) {
    // Първо мащабиране (S), после преместване (T):  M = T * S
    return glm::scale(glm::translate(glm::mat4(1.0f), center), size);
}
