#include "Sky.h"
#include "DayNight.h"

Sky::Sky() {
    m_shader = std::make_unique<Shader>(ASSET_DIR "shaders/sky.vert", ASSET_DIR "shaders/sky.frag");
    m_dome.reset(Primitives::createSphere(32, 16));
}

void Sky::draw(const glm::mat4& view, const glm::mat4& projection, const DayNight& dn) const {
    // mat4(mat3(view)) - запазва само завъртането на камерата, без преместването
    glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));

    m_shader->use();
    m_shader->setMat4("uView",       viewNoTranslation);
    m_shader->setMat4("uProjection", projection);
    m_shader->setVec3("uZenith",     dn.zenithColor());
    m_shader->setVec3("uHorizon",    dn.horizonColor());
    m_shader->setVec3("uSunDir",     dn.sunDirection());
    m_shader->setVec3("uMoonDir",    dn.moonDirection());
    m_shader->setVec3("uSunColor",   dn.sunColor());
    m_shader->setFloat("uNight",     dn.nightFactor());

    // Гледаме сферата ОТВЪТРЕ -> виждаме задните ѝ стени, затова
    // изключваме отрязването им. LEQUAL - защото дълбочината на небето
    // е точно 1.0, колкото е и стойността, с която се изчиства Z-буферът.
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);
    m_dome->draw();
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
}
