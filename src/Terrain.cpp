#include "Terrain.h"
#include "Road.h"

#include <algorithm>
#include <cmath>

// ============================================================
//  Процедурен шум
// ============================================================
namespace {

// Хеш функция: цели координати -> псевдослучайно число в [0, 1).
// Една и съща точка винаги дава една и съща стойност.
float hash2D(int x, int z) {
    unsigned int h = static_cast<unsigned int>(x) * 374761393u
                   + static_cast<unsigned int>(z) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (h & 0xFFFFFF) / float(0x1000000);
}

// Value noise: случайни стойности във възлите на целочислена мрежа,
// интерполирани гладко между тях.
float valueNoise(float x, float z) {
    int   xi = static_cast<int>(std::floor(x));
    int   zi = static_cast<int>(std::floor(z));
    float tx = x - xi, tz = z - zi;

    // smoothstep - производната е 0 във възлите -> няма видими "ръбове"
    tx = tx * tx * (3.0f - 2.0f * tx);
    tz = tz * tz * (3.0f - 2.0f * tz);

    float a = hash2D(xi,     zi    );
    float b = hash2D(xi + 1, zi    );
    float c = hash2D(xi,     zi + 1);
    float d = hash2D(xi + 1, zi + 1);
    return glm::mix(glm::mix(a, b, tx), glm::mix(c, d, tx), tz);
}

// fBm (fractal Brownian motion): сума от няколко "октави" шум.
// Всяка следваща октава е с 2 пъти по-висока честота (по-ситни детайли)
// и 2 пъти по-малка амплитуда. Резултатът наподобява естествен релеф.
float fbm(float x, float z, int octaves = 5) {
    float sum = 0.0f, amp = 1.0f, freq = 1.0f, norm = 0.0f;
    for (int i = 0; i < octaves; ++i) {
        sum  += valueNoise(x * freq, z * freq) * amp;
        norm += amp;
        amp  *= 0.5f;
        freq *= 2.0f;
    }
    return sum / norm;   // нормализирано в [0, 1)
}

float smoothstep(float e0, float e1, float x) {
    float t = std::clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

} // namespace

// ============================================================
//  Генериране на терена
// ============================================================
Terrain::Terrain(float size, int resolution, const Road& road)
    : m_size(size), m_res(resolution), m_step(size / resolution) {

    const int   n    = m_res + 1;          // върхове по една страна
    const float half = m_size * 0.5f;

    // ---- 1. Височини (height map) ----
    m_heights.resize(n * n);
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n; ++i) {
            float x = -half + i * m_step;
            float z = -half + j * m_step;

            // Хълмове: шум с "дължина на вълната" ~70 м, височина до ~22 м
            float hills = fbm(x / 70.0f, z / 70.0f) * 22.0f;

            // Планини по края на света
            float edge     = std::max(std::abs(x), std::abs(z));
            float mountain = smoothstep(half * 0.72f, half, edge) * 45.0f;

            // Маска около пътя: 0 върху пътя (+3 м банкет), плавно до 1 след ~35 м
            float d    = road.distanceToCenter(x, z);
            float mask = smoothstep(road.halfWidth() + 3.0f, road.halfWidth() + 35.0f, d);

            m_heights[j * n + i] = (hills + mountain) * mask;
        }
    }

    // ---- 2. Върхове: позиция, нормала, UV ----
    std::vector<Vertex> verts;
    verts.reserve(n * n);
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n; ++i) {
            float x = -half + i * m_step;
            float z = -half + j * m_step;

            // Нормала чрез централни разлики: наклонът по X и по Z се
            // оценява от съседните върхове.
            float hL = gridHeight(i - 1, j), hR = gridHeight(i + 1, j);
            float hD = gridHeight(i, j - 1), hU = gridHeight(i, j + 1);
            glm::vec3 normal = glm::normalize(glm::vec3(hL - hR, 2.0f * m_step, hD - hU));

            // UV в метри / 8 -> текстурата на тревата се повтаря на всеки 8 м
            glm::vec2 uv(x / 8.0f, z / 8.0f);

            verts.push_back({{x, gridHeight(i, j), z}, normal, uv});
        }
    }

    // ---- 3. Индекси: всяка клетка от мрежата = 2 триъгълника ----
    std::vector<unsigned int> idx;
    idx.reserve(m_res * m_res * 6);
    for (int j = 0; j < m_res; ++j) {
        for (int i = 0; i < m_res; ++i) {
            unsigned int a = j * n + i;        // долу-ляво
            unsigned int b = a + 1;            // долу-дясно
            unsigned int c = a + n;            // горе-ляво
            unsigned int d = c + 1;            // горе-дясно
            // обходени обратно на часовниковата стрелка, гледано отгоре (+Y)
            idx.insert(idx.end(), {a, c, b,  b, c, d});
        }
    }
    m_mesh = std::make_unique<Mesh>(verts, idx);
}

float Terrain::gridHeight(int i, int j) const {
    i = std::clamp(i, 0, m_res);
    j = std::clamp(j, 0, m_res);
    return m_heights[j * (m_res + 1) + i];
}

// ------------------------------------------------------------
//  Билинейна интерполация: точката (x, z) попада в клетка с
//  четири ъгъла; височината е претеглена сума от тях.
// ------------------------------------------------------------
float Terrain::heightAt(float x, float z) const {
    float gx = (x + m_size * 0.5f) / m_step;   // координати в "клетки"
    float gz = (z + m_size * 0.5f) / m_step;
    int   i  = static_cast<int>(std::floor(gx));
    int   j  = static_cast<int>(std::floor(gz));
    float tx = gx - i, tz = gz - j;

    float h00 = gridHeight(i,     j    );
    float h10 = gridHeight(i + 1, j    );
    float h01 = gridHeight(i,     j + 1);
    float h11 = gridHeight(i + 1, j + 1);
    return glm::mix(glm::mix(h00, h10, tx), glm::mix(h01, h11, tx), tz);
}
