#include "World.h"
#include "Road.h"
#include "Terrain.h"
#include "Car.h"
#include "Shader.h"
#include "Renderer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <random>

namespace {

// Матрица от локална координатна система: X = xAxis (хоризонтален),
// Y = нагоре, Z = X x Y, начало = origin. (Колоните = образите на осите.)
glm::mat4 frameMatrix(const glm::vec3& xAxis, const glm::vec3& origin) {
    glm::vec3 x = glm::normalize(xAxis);
    glm::vec3 y(0.0f, 1.0f, 0.0f);
    glm::vec3 z = glm::cross(x, y);
    glm::mat4 m(1.0f);
    m[0] = glm::vec4(x, 0.0f);
    m[1] = glm::vec4(y, 0.0f);
    m[2] = glm::vec4(z, 0.0f);
    m[3] = glm::vec4(origin, 1.0f);
    return m;
}

glm::vec2 xz(const glm::vec3& v) { return {v.x, v.z}; }

Material makeMaterial(const glm::vec3& color, float spec, float shin,
                      const glm::vec3& emissive = glm::vec3(0.0f)) {
    Material m;
    m.color = color; m.specular = spec; m.shininess = shin; m.emissive = emissive;
    return m;
}

// Части на уличната лампа (локално: стълбът по +Y, рамото по +X към пътя)
const glm::vec3 kLampLightLocal(1.9f, 5.65f, 0.0f);

} // namespace

// ============================================================
//  Създаване на света
// ============================================================
World::World(const Road& road, const Terrain& terrain) : m_terrain(&terrain) {
    m_cube.reset(Primitives::createCube());
    m_cylinder.reset(Primitives::createCylinder());
    m_sphere.reset(Primitives::createSphere());
    m_cone.reset(Primitives::createCone());

    // Сградите се създават ПРЕДИ дърветата - дърветата ги заобикалят
    createLampsAndProps(road, terrain);
    createBuildings(road, terrain);
    createGuardrails(road, terrain);
    createCones(road, terrain);
    createTrees(road, terrain);
}

// ------------------------------------------------------------
//  Улични лампи, бали слама, декоративни сфери
// ------------------------------------------------------------
void World::createLampsAndProps(const Road& road, const Terrain& terrain) {
    const float hw = road.halfWidth();

    // Лампи - през 20 точки от централната линия, от дясната страна
    for (int i = 0; i < road.sampleCount() && (int)m_lamps.size() < kMaxPointLights; i += 20) {
        glm::vec3 right = road.rightVector(i);
        glm::vec3 base  = road.centerPoint(i) + right * (hw + 1.5f);
        base.y = terrain.heightAt(base.x, base.z);

        glm::mat4 m = frameMatrix(-right, base);          // рамото сочи към пътя
        m_lamps.push_back(m);

        PointLight pl;
        pl.position = glm::vec3(m * glm::vec4(kLampLightLocal, 1.0f));
        m_lampLights.push_back(pl);

        m_staticCircles.push_back({xz(base), 0.2f});      // стълбът
    }

    // Бали слама - защитна стена от дясната страна (точки 92..104)
    Material straw = makeMaterial({0.85f, 0.72f, 0.38f}, 0.05f, 8.0f);
    for (int i = 92; i <= 104; ++i) {
        glm::vec3 p = road.centerPoint(i) + road.rightVector(i) * (road.halfWidth() + 4.0f);
        p.y = terrain.heightAt(p.x, p.z);
        // Цилиндърът е с ос Y -> завъртаме на 90° около локалното Z,
        // за да легне по посоката на пътя
        glm::mat4 m = frameMatrix(road.direction(i), p);
        m = glm::translate(m, {0.0f, 0.7f, 0.0f});
        m = glm::rotate(m, glm::radians(90.0f), {0.0f, 0.0f, 1.0f});
        m = glm::scale(m, {1.4f, 1.3f, 1.4f});
        m_props.push_back({m_cylinder.get(), m, straw});
        m_staticCircles.push_back({xz(p), 0.8f});
    }

    // Декоративни сфери от Етап 3 (различна shininess)
    {
        glm::vec3 c0 = road.centerPoint(6), r0 = road.rightVector(6), d0 = road.direction(6);
        const float shin[3] = {8.0f, 32.0f, 128.0f};
        const glm::vec3 col[3] = {{0.80f, 0.15f, 0.12f}, {0.15f, 0.45f, 0.85f}, {0.90f, 0.70f, 0.20f}};
        for (int k = 0; k < 3; ++k) {
            glm::vec3 p = c0 - r0 * (hw + 3.0f) + d0 * (k * 5.0f);
            p.y = terrain.heightAt(p.x, p.z) + 1.0f;
            glm::mat4 m = glm::scale(glm::translate(glm::mat4(1.0f), p), glm::vec3(2.0f));
            m_props.push_back({m_sphere.get(), m, makeMaterial(col[k], 0.8f, shin[k])});
            m_staticCircles.push_back({xz(p), 1.0f});
        }
    }
}

// ------------------------------------------------------------
//  Сгради - отляво на пътя, ориентирани по посоката му
// ------------------------------------------------------------
void World::createBuildings(const Road& road, const Terrain& terrain) {
    struct Spec { int index; glm::vec3 size; glm::vec3 color; };
    const Spec specs[] = {
        { 30, {12.0f, 5.0f, 8.0f}, {0.90f, 0.86f, 0.75f}},
        { 84, {10.0f, 7.0f, 8.0f}, {0.75f, 0.42f, 0.32f}},
        {118, {14.0f, 4.5f, 9.0f}, {0.82f, 0.82f, 0.85f}},
        {178, {11.0f, 6.0f, 8.0f}, {0.92f, 0.80f, 0.55f}},
        {232, { 9.0f, 5.0f, 7.0f}, {0.70f, 0.75f, 0.68f}},
    };

    for (const Spec& s : specs) {
        glm::vec3 dir   = road.direction(s.index);
        glm::vec3 right = road.rightVector(s.index);
        float len = s.size.x, h = s.size.y, wid = s.size.z;

        glm::vec3 c = road.centerPoint(s.index) - right * (road.halfWidth() + 6.0f + wid * 0.5f);

        // Най-ниската и най-високата точка на терена под сградата:
        // сградата "влиза" в земята до най-ниската, за да не виси във въздуха
        float gMin = 1e9f, gMax = -1e9f;
        for (int a = -1; a <= 1; ++a)
            for (int b = -1; b <= 1; ++b) {
                glm::vec3 p = c + dir * (a * len * 0.5f) + right * (b * wid * 0.5f);
                float g = terrain.heightAt(p.x, p.z);
                gMin = std::min(gMin, g);
                gMax = std::max(gMax, g);
            }

        Building bld;
        bld.frame     = frameMatrix(dir, glm::vec3(c.x, 0.0f, c.z));
        bld.size      = glm::vec3(len, gMax + h, wid);      // size.y = височината на покрива
        bld.baseY     = gMin - 1.0f;
        bld.wallColor = s.color;
        bld.box       = OBB{xz(c), {len * 0.5f, wid * 0.5f}, std::atan2(dir.z, dir.x)};
        m_buildings.push_back(bld);
        m_staticBoxes.push_back(bld.box);
    }
}

// ------------------------------------------------------------
//  Мантинели - отляво на пътя в три участъка
// ------------------------------------------------------------
void World::createGuardrails(const Road& road, const Terrain& terrain) {
    const int ranges[3][2] = {{42, 70}, {128, 162}, {196, 226}};
    const float offset = road.halfWidth() + 1.2f;

    for (const auto& r : ranges) {
        for (int i = r[0]; i < r[1]; ++i) {
            glm::vec3 a = road.centerPoint(i)     - road.rightVector(i)     * offset;
            glm::vec3 b = road.centerPoint(i + 1) - road.rightVector(i + 1) * offset;
            a.y = terrain.heightAt(a.x, a.z);
            b.y = terrain.heightAt(b.x, b.z);

            glm::vec3 seg = b - a;
            float len = glm::length(glm::vec2(seg.x, seg.z));
            glm::vec3 mid = (a + b) * 0.5f;

            RailSegment rs;
            rs.model = frameMatrix(seg, mid) * partTransform({0.0f, 0.62f, 0.0f}, {len + 0.05f, 0.30f, 0.06f});
            rs.post  = frameMatrix(seg, a)   * partTransform({0.0f, 0.40f, 0.0f}, {0.10f, 0.80f, 0.10f});
            rs.box   = OBB{xz(mid), {len * 0.5f + 0.02f, 0.15f}, std::atan2(seg.z, seg.x)};
            m_rails.push_back(rs);
            m_staticBoxes.push_back(rs.box);
        }
    }
}

// ------------------------------------------------------------
//  Конуси - слалом след старта
// ------------------------------------------------------------
void World::createCones(const Road& road, const Terrain& terrain) {
    int k = 0;
    for (int i = 10; i <= 24; i += 2, ++k) {
        float side = (k % 2 == 0) ? 2.5f : -2.5f;
        glm::vec3 p = road.centerPoint(i) + road.rightVector(i) * side;
        p.y = terrain.heightAt(p.x, p.z) + Road::kSurfaceY;
        Cone c;
        c.home = c.pos = p;
        m_cones.push_back(c);
    }
}

// ------------------------------------------------------------
//  Дървета - случайни позиции, но далеч от пътя и сградите.
//  Генераторът е с фиксиран seed -> гората е еднаква при всяко стартиране.
// ------------------------------------------------------------
void World::createTrees(const Road& road, const Terrain& terrain) {
    std::mt19937 rng(20241102);
    std::uniform_real_distribution<float> coord(-232.0f, 232.0f);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);

    std::vector<glm::mat4> pines, oaks;
    std::vector<glm::vec2> placed;
    const int kTarget = 380;

    for (int attempt = 0; attempt < 6000 && (int)placed.size() < kTarget; ++attempt) {
        glm::vec2 p(coord(rng), coord(rng));

        // 1) Не по-близо от 7 м до ръба на пътя
        if (road.distanceToCenter(p.x, p.y) < road.halfWidth() + 7.0f) continue;

        // 2) Не върху сградите
        bool blocked = false;
        for (const Building& b : m_buildings)
            if (glm::length(p - b.box.center) < glm::length(b.box.half) + 4.0f) { blocked = true; break; }
        if (blocked) continue;

        // 3) Не прекалено близо едно до друго
        for (const glm::vec2& q : placed)
            if (glm::dot(p - q, p - q) < 3.5f * 3.5f) { blocked = true; break; }
        if (blocked) continue;

        // 4) Не по върховете на планините
        float y = terrain.heightAt(p.x, p.y);
        if (y > 32.0f) continue;

        float scale = 0.8f + 0.6f * unit(rng);
        float angle = unit(rng) * 6.2831853f;

        // Матрица на копието: T * R * S (равномерен мащаб)
        glm::mat4 m = glm::translate(glm::mat4(1.0f), {p.x, y - 0.2f, p.y});
        m = glm::rotate(m, angle, {0.0f, 1.0f, 0.0f});
        m = glm::scale(m, glm::vec3(scale));

        (unit(rng) < 0.5f ? pines : oaks).push_back(m);
        placed.push_back(p);
        m_staticCircles.push_back({p, 0.35f * scale});     // стволът
    }

    // Всеки вид дърво = 2 mesh-а (ствол + корона), всеки с буфер от копия
    m_pineTrunk.reset(Primitives::createCylinder(10));
    m_pineCrown.reset(Primitives::createCone(16));
    m_oakTrunk .reset(Primitives::createCylinder(10));
    m_oakCrown .reset(Primitives::createSphere(16, 10));
    m_pineTrunk->setInstances(pines);
    m_pineCrown->setInstances(pines);
    m_oakTrunk ->setInstances(oaks);
    m_oakCrown ->setInstances(oaks);
    m_pineCount = pines.size();
    m_oakCount  = oaks.size();
}

// ============================================================
//  Анимация на съборените конуси
// ============================================================
void World::update(float dt) {
    for (Cone& c : m_cones) {
        if (!c.knocked) continue;
        c.pos.x += c.velocity.x * dt;
        c.pos.z += c.velocity.y * dt;
        c.velocity *= std::exp(-2.5f * dt);                  // триене в земята
        c.tilt = std::min(90.0f, c.tilt + 360.0f * dt);      // пада за 0.25 s
        c.pos.y = m_terrain->heightAt(c.pos.x, c.pos.z) + Road::kSurfaceY;
    }
}

void World::reset() {
    for (Cone& c : m_cones) {
        c.pos = c.home;
        c.velocity = glm::vec2(0.0f);
        c.tilt = 0.0f;
        c.knocked = false;
    }
}

// ============================================================
//  Колизии
//
//  Двуфазна проверка:
//    broad phase  - бързо отхвърляне по описаните окръжности
//                   (повечето от ~700 обекта са далеч от колата)
//    narrow phase - точната проверка OBB/кръг или OBB/OBB (SAT)
// ============================================================
float World::collide(Car& car) {
    float maxImpact = 0.0f;
    OBB   box  = car.collisionBox();
    float carR = box.boundingRadius();

    // Две итерации: ако избутването от един обект вкара колата в друг
    // (напр. между дърво и мантинела), втората итерация го оправя
    for (int iter = 0; iter < 2; ++iter) {
        for (const Circle& c : m_staticCircles) {
            if (!boundingCirclesOverlap(box.center, carR, c.center, c.radius)) continue;
            Contact ct = obbVsCircle(box, c);
            if (!ct.hit) continue;
            maxImpact = std::max(maxImpact, car.applyCollision(ct.normal, ct.depth, 0.3f));
            box = car.collisionBox();
        }
        for (const OBB& b : m_staticBoxes) {
            if (!boundingCirclesOverlap(box.center, carR, b.center, b.boundingRadius())) continue;
            Contact ct = obbVsObb(box, b);
            if (!ct.hit) continue;
            maxImpact = std::max(maxImpact, car.applyCollision(ct.normal, ct.depth, 0.2f));
            box = car.collisionBox();
        }
    }

    // Конусите са леки: колата не спира, а ги избутва
    for (Cone& c : m_cones) {
        if (c.knocked) continue;
        Circle shape{xz(c.pos), 0.3f};
        if (!boundingCirclesOverlap(box.center, carR, shape.center, shape.radius)) continue;
        Contact ct = obbVsCircle(box, shape);
        if (!ct.hit) continue;

        c.knocked  = true;
        // ct.normal сочи от конуса към колата -> конусът отлита в обратната посока
        c.velocity = car.velocity2D() * 1.2f - ct.normal * 3.0f;
        float len  = glm::length(c.velocity);
        c.fallDir  = len > 0.01f ? c.velocity / len : -ct.normal;
        car.scaleSpeed(0.95f);                                // малка загуба на скорост
        maxImpact  = std::max(maxImpact, 1.0f);
    }
    return maxImpact;
}

// ============================================================
//  Рисуване
// ============================================================
void World::draw(const Shader& shader, bool lampsOn, float night) const {
    const Material metal  = makeMaterial({0.35f, 0.37f, 0.40f}, 0.6f, 64.0f);
    const Material rail   = makeMaterial({0.72f, 0.74f, 0.78f}, 0.7f, 48.0f);
    const Material bulb   = makeMaterial({1.0f, 0.95f, 0.8f}, 0.0f, 1.0f,
                                          lampsOn ? glm::vec3(1.6f, 1.4f, 0.9f) : glm::vec3(0.0f));
    const Material bark   = makeMaterial({0.33f, 0.22f, 0.13f}, 0.05f, 8.0f);
    const Material needle = makeMaterial({0.12f, 0.30f, 0.14f}, 0.10f, 16.0f);
    const Material leaves = makeMaterial({0.22f, 0.45f, 0.16f}, 0.10f, 16.0f);
    const Material roof   = makeMaterial({0.45f, 0.18f, 0.14f}, 0.2f, 16.0f);
    const Material door   = makeMaterial({0.30f, 0.20f, 0.12f}, 0.2f, 16.0f);
    const Material window = makeMaterial({0.10f, 0.14f, 0.20f}, 1.0f, 128.0f);
    Material litWindow    = window;           // светещ прозорец нощем (топла светлина)
    litWindow.emissive    = glm::vec3(1.0f, 0.78f, 0.42f) * night;
    const Material orange = makeMaterial({1.0f, 0.38f, 0.05f}, 0.4f, 32.0f);
    const Material white  = makeMaterial({0.95f, 0.95f, 0.95f}, 0.4f, 32.0f);
    const Material black  = makeMaterial({0.08f, 0.08f, 0.08f}, 0.1f, 8.0f);

    // ---- Дървета: 4 заявки за рисуване за ~380 дървета (инстанциране) ----
    drawMeshInstanced(shader, *m_pineTrunk, partTransform({0.0f, 1.0f, 0.0f}, {0.35f, 2.0f, 0.35f}), bark,   MODE_COLOR);
    drawMeshInstanced(shader, *m_pineCrown, partTransform({0.0f, 3.9f, 0.0f}, {3.2f, 5.2f, 3.2f}),   needle, MODE_COLOR);
    drawMeshInstanced(shader, *m_oakTrunk,  partTransform({0.0f, 1.4f, 0.0f}, {0.45f, 2.8f, 0.45f}), bark,   MODE_COLOR);
    drawMeshInstanced(shader, *m_oakCrown,  partTransform({0.0f, 4.2f, 0.0f}, {3.8f, 3.4f, 3.8f}),   leaves, MODE_COLOR);

    // ---- Улични лампи ----
    for (const glm::mat4& lamp : m_lamps) {
        drawMesh(shader, *m_cube, lamp * partTransform({0.0f, 3.0f,  0.0f}, {0.18f, 6.0f, 0.18f}), metal, MODE_COLOR);
        drawMesh(shader, *m_cube, lamp * partTransform({1.0f, 5.95f, 0.0f}, {2.0f, 0.12f, 0.12f}), metal, MODE_COLOR);
        drawMesh(shader, *m_cube, lamp * partTransform({1.9f, 5.85f, 0.0f}, {0.7f, 0.15f, 0.35f}), metal, MODE_COLOR);
        drawMesh(shader, *m_cube, lamp * partTransform({1.9f, 5.75f, 0.0f}, {0.55f, 0.06f, 0.25f}), bulb, MODE_COLOR);
    }

    // ---- Сгради ----
    for (const Building& b : m_buildings) {
        float len = b.size.x, top = b.size.y, wid = b.size.z;
        float bodyH = top - b.baseY;
        Material wall = makeMaterial(b.wallColor, 0.1f, 16.0f);

        // Тяло: от baseY (под земята) до top
        drawMesh(shader, *m_cube, b.frame * partTransform({0.0f, b.baseY + bodyH * 0.5f, 0.0f}, {len, bodyH, wid}),
                 wall, MODE_COLOR);

        // Двускатен покрив: куб, завъртян на 45° около дългата си ос.
        // Долната половина на "ромба" е скрита в сградата, горната образува покрива.
        float side = (wid + 0.8f) / std::sqrt(2.0f);
        glm::mat4 r = glm::translate(b.frame, {0.0f, top, 0.0f});
        r = glm::rotate(r, glm::radians(45.0f), {1.0f, 0.0f, 0.0f});
        r = glm::scale(r, {len + 0.6f, side, side});
        drawMesh(shader, *m_cube, r, roof, MODE_COLOR);

        // Фасада към пътя (локално +Z): врата и прозорци
        float zFace = wid * 0.5f + 0.03f;
        glm::vec3 faceWorld = glm::vec3(b.frame * glm::vec4(0.0f, 0.0f, zFace, 1.0f));
        float ground = m_terrain->heightAt(faceWorld.x, faceWorld.z);
        drawMesh(shader, *m_cube, b.frame * partTransform({0.0f, ground + 1.1f, zFace}, {1.2f, 2.2f, 0.06f}),
                 door, MODE_COLOR);
        for (float x = -len * 0.5f + 1.8f; x < len * 0.5f - 1.0f; x += 2.6f) {
            if (std::abs(x) < 1.2f) continue;               // място за вратата
            for (float y = ground + 1.6f; y < top - 1.0f; y += 2.2f) {
                // Около 60% от прозорците светят нощем. Кои - определя
                // "хеш" от позицията, за да е винаги едно и също.
                float hsh = std::fmod(std::abs(std::sin(x * 12.9898f + y * 78.233f + b.size.x) * 43758.5453f), 1.0f);
                drawMesh(shader, *m_cube, b.frame * partTransform({x, y, zFace}, {1.1f, 1.1f, 0.06f}),
                         hsh < 0.6f ? litWindow : window, MODE_COLOR);
            }
        }
    }

    // ---- Мантинели ----
    for (const RailSegment& r : m_rails) {
        drawMesh(shader, *m_cube, r.model, rail, MODE_COLOR);
        drawMesh(shader, *m_cube, r.post,  rail, MODE_COLOR);
    }

    // ---- Бали и сфери ----
    for (const Prop& p : m_props)
        drawMesh(shader, *p.mesh, p.model, p.material, MODE_COLOR);

    // ---- Конуси ----
    for (const Cone& c : m_cones) {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), c.pos);
        if (c.tilt > 0.0f) {
            // Падане: завъртане около хоризонтална ос, перпендикулярна на
            // посоката на падане -> върхът на конуса се накланя натам
            glm::vec3 fall(c.fallDir.x, 0.0f, c.fallDir.y);
            glm::vec3 axis = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), fall);
            m = glm::translate(m, {0.0f, 0.2f * c.tilt / 90.0f, 0.0f});  // легнал е малко по-високо
            m = glm::rotate(m, glm::radians(c.tilt), axis);
        }
        drawMesh(shader, *m_cube,     m * partTransform({0.0f, 0.025f, 0.0f}, {0.50f, 0.05f, 0.50f}), black,  MODE_COLOR);
        drawMesh(shader, *m_cone,     m * partTransform({0.0f, 0.425f, 0.0f}, {0.42f, 0.75f, 0.42f}), orange, MODE_COLOR);
        drawMesh(shader, *m_cylinder, m * partTransform({0.0f, 0.45f,  0.0f}, {0.21f, 0.08f, 0.21f}), white,  MODE_COLOR);
    }
}
