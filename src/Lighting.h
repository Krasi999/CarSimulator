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

// Максимален брой светлини - трябва да съвпада с MAX_POINT_LIGHTS / MAX_SPOT_LIGHTS в basic.frag
constexpr int kMaxPointLights = 16;
constexpr int kMaxSpotLights  = 4;

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
    glm::vec3 color   {2.8f, 2.3f, 1.6f};    // > 1 -> по-силна от "бяла" светлина
    float constant  = 1.0f;     // Kc
    float linear    = 0.14f;    // Kl
    float quadratic = 0.07f;    // Kq   -> полезен обхват ~30 м: ясно петно под лампата
};

// Прожекторна светлина (фар): точкова светлина, ограничена в конус.
// Вътре в innerCutoff - пълна сила, между inner и outer - плавно
// отслабва, извън outerCutoff - тъмно.
struct SpotLight {
    glm::vec3 position{0.0f};
    glm::vec3 direction{1.0f, 0.0f, 0.0f};    // накъде свети (единичен вектор)
    // Силна светлина: фарът е ниско (0.68 м) и лъчите падат почти успоредно
    // на пътя -> N·L е малко (~0.1), затова цветът е много над 1
    glm::vec3 color{12.0f, 11.4f, 10.0f};
    float innerCutoff = 0.985f;               // cos(10°)
    float outerCutoff = 0.906f;               // cos(25°)
    float constant  = 1.0f;
    float linear    = 0.05f;
    float quadratic = 0.01f;
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
    bool lamps    = true;       // разрешение за лампите (включват се автоматично нощем)
    bool blinn    = false;       // false = класически Phong, true = Blinn-Phong
};

// Изпращане на данните към шейдъра
void uploadLights(const Shader& shader, const DirLight& sun,
                  const std::vector<PointLight>& lamps, const std::vector<SpotLight>& spots,
                  const LightingToggles& t);
void uploadMaterial(const Shader& shader, const Material& m);
