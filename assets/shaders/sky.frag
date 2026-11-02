#version 330 core
// ============================================================
//  Небесна сфера - fragment шейдър.
//  Цветът се изчислява само от ПОСОКАТА на погледа (vDir).
// ============================================================

in vec3 vDir;

uniform vec3  uZenith;      // цвят високо в небето
uniform vec3  uHorizon;     // цвят на хоризонта (= цвят на мъглата)
uniform vec3  uSunDir;
uniform vec3  uMoonDir;
uniform vec3  uSunColor;
uniform float uNight;       // 0 ден .. 1 нощ

out vec4 FragColor;

// Псевдослучайно число за клетка от решетка - за звездите
float hash(vec3 p) {
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

void main() {
    vec3  d = normalize(vDir);
    float h = d.y;                                   // височина над хоризонта: -1 .. 1

    // ---- Градиент: хоризонт -> зенит ----
    // pow(h, 0.5) - цветът на хоризонта заема по-тясна ивица
    vec3 color = mix(uHorizon, uZenith, pow(clamp(h, 0.0, 1.0), 0.5));
    // Под хоризонта (вижда се рядко - зад планините) - малко по-тъмно
    if (h < 0.0) color = uHorizon * (1.0 + h * 0.5);

    // ---- Слънце: остър диск + широк ореол ----
    // dot(d, sunDir) = cos на ъгъла между погледа и слънцето
    float s = max(dot(d, uSunDir), 0.0);
    color += uSunColor * (smoothstep(0.9994, 0.9997, s) * 6.0     // диск
                        + pow(s, 12.0) * 0.35);                    // ореол

    // ---- Луна ----
    float m = max(dot(d, uMoonDir), 0.0);
    color += vec3(0.85, 0.88, 0.95) * smoothstep(0.9993, 0.9996, m) * uNight;

    // ---- Звезди ----
    // Посоката се "нарязва" на малки клетки; рядко някоя клетка свети.
    if (uNight > 0.01 && h > 0.0) {
        float star = step(0.9975, hash(floor(d * 250.0)));
        // smoothstep(0.5, 1, uNight): звездите се появяват едва при пълна нощ
        color += vec3(star) * smoothstep(0.5, 1.0, uNight) * smoothstep(0.0, 0.25, h) * 0.9;
    }

    FragColor = vec4(color, 1.0);
}
