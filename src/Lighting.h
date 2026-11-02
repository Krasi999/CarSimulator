#pragma once
// ============================================================
//  Lighting - структури за светлините и материалите и
//  изпращането им към шейдъра като uniform променливи.
//
//  Моделът на Phong разделя отразената светлина на 3 компонента:
//    Ambient  (фоново)    - разсеяна светлина от околната среда
//    Diffuse  (дифузно)   - матово отражение, зависи от ъгъла
//                           между нормалата и посоката към светлината
//    Specular (огледално) - блясък, зависи от позицията на наблюдателя
// ============================================================
#include <glm/glm.hpp>
#include <vector>

class Shader;

// Максимален брой точкови светлини - трябва да съвпада с MAX_POINT_LIGHTS в basic.frag
constexpr int kMaxPointLights = 16;

// Насочена светлина (слънце): безкрайно далечна, всички лъчи са успоредни
struct DirLight {
    glm::vec3 direction{0.4f, 1.0f, 0.3f};   // посока КЪМ светлината
    glm::vec3 color    {1.0f, 0.96f, 0.88f};  // цвят и сила на светлината
    glm::vec3 ambient  {0.30f, 0.34f, 0.40f}; // фоново осветяване (синкаво от небето)
};

// Точкова светлина (лампа): свети във всички посоки от една точка,
// интензитетът намалява с разстоянието (затихване / attenuation)
struct PointLight {
    glm::vec3 position{0.0f};
    glm::vec3 color   {1.0f, 0.85f, 0.6f};
    float constant  = 1.0f;     // Kc
    float linear    = 0.09f;    // Kl
    float quadratic = 0.032f;   // Kq   -> полезен обхват ~50 м
};

// Материал: как повърхността отразява светлината
struct Material {
    glm::vec3 color{1.0f};       // основен цвят (умножава се с текстурата, ако има)
    float specular  = 0.1f;      // сила на блясъка (0 = напълно матово)
    float shininess = 16.0f;     // степен на блясъка: по-голяма -> по-малко и по-остро петно
    glm::vec3 emissive{0.0f};    // собствено светене (напр. крушката на лампата)
};

// Кои компоненти на осветлението са включени (за демонстрация)
struct LightingToggles {
    bool ambient  = true;
    bool diffuse  = true;
    bool specular = true;
    bool sun      = true;
    bool lamps    = true;
    bool blinn    = false;       // false = класически Phong, true = Blinn-Phong
};

// Изпращане на данните към шейдъра
void uploadLights(const Shader& shader, const DirLight& sun,
                  const std::vector<PointLight>& lamps, const LightingToggles& t);
void uploadMaterial(const Shader& shader, const Material& m);
