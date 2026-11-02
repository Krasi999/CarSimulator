#version 330 core
// ============================================================
//  Fragment шейдър - изпълнява се ЗА ВСЕКИ ПИКСЕЛ (фрагмент).
//  Етап 1: опростено осветяване, за да се различават стените
//  на обектите. В Етап 3 ще бъде заменено с пълен модел на Phong.
// ============================================================

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;

uniform vec3 uColor;        // основен цвят на обекта
uniform bool uChecker;      // true -> шахматна шарка (за земята)
uniform vec3 uCameraPos;    // позиция на камерата (за мъглата)

out vec4 FragColor;

void main() {
    vec3 color = uColor;

    // Шахматна шарка 2x2 метра - помага да се усети движението
    if (uChecker) {
        float c = mod(floor(vWorldPos.x / 2.0) + floor(vWorldPos.z / 2.0), 2.0);
        color *= mix(0.85, 1.0, c);
    }

    // Временно осветяване от фиксирана посока
    vec3 lightDir = normalize(vec3(0.4, 1.0, 0.3));
    float diffuse = max(dot(normalize(vNormal), lightDir), 0.0);
    color *= 0.35 + 0.65 * diffuse;

    // Линейна мъгла - далечните обекти плавно се сливат с небето
    vec3  skyColor = vec3(0.55, 0.75, 0.95);
    float dist = length(vWorldPos - uCameraPos);
    float fog  = clamp((dist - 60.0) / (200.0 - 60.0), 0.0, 1.0);
    color = mix(color, skyColor, fog);

    FragColor = vec4(color, 1.0);
}
