#include "Lighting.h"
#include "Shader.h"

#include <algorithm>
#include <string>

void uploadLights(const Shader& shader, const DirLight& sun,
                  const std::vector<PointLight>& lamps, const LightingToggles& t) {
    // Слънце. Изключеното слънце = черна светлина (без да се променя шейдърът).
    shader.setVec3("uSun.direction", glm::normalize(sun.direction));
    shader.setVec3("uSun.color",     t.sun ? sun.color : glm::vec3(0.0f));
    shader.setVec3("uSun.ambient",   sun.ambient);

    // Точкови светлини. Uniform масивите в GLSL се адресират по име
    // с индекс: "uPointLights[3].position".
    int count = t.lamps ? std::min<int>(static_cast<int>(lamps.size()), kMaxPointLights) : 0;
    shader.setInt("uNumPointLights", count);
    for (int i = 0; i < count; ++i) {
        std::string base = "uPointLights[" + std::to_string(i) + "].";
        shader.setVec3 ((base + "position").c_str(),  lamps[i].position);
        shader.setVec3 ((base + "color").c_str(),     lamps[i].color);
        shader.setFloat((base + "constant").c_str(),  lamps[i].constant);
        shader.setFloat((base + "linear").c_str(),    lamps[i].linear);
        shader.setFloat((base + "quadratic").c_str(), lamps[i].quadratic);
    }

    // Превключватели: всеки компонент се умножава по 1 или 0
    shader.setVec3("uComponents", glm::vec3(t.ambient  ? 1.0f : 0.0f,
                                            t.diffuse  ? 1.0f : 0.0f,
                                            t.specular ? 1.0f : 0.0f));
    shader.setInt("uBlinn", t.blinn ? 1 : 0);
}

void uploadMaterial(const Shader& shader, const Material& m) {
    shader.setVec3 ("uMaterial.color",     m.color);
    shader.setFloat("uMaterial.specular",  m.specular);
    shader.setFloat("uMaterial.shininess", m.shininess);
    shader.setVec3 ("uMaterial.emissive",  m.emissive);
}
