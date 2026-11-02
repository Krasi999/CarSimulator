#version 330 core
// ============================================================
//  Fragment шейдър - изпълнява се ЗА ВСЕКИ ПИКСЕЛ (фрагмент).
//
//  Етап 2: текстуриране.
//    uMode = 0 -> само цвят (uColor)
//    uMode = 1 -> текстура (uTexture) x uColor
//    uMode = 2 -> терен: трева + скала, смесени според наклона
//
//  Осветлението все още е опростено (само дифузно).
//  В Етап 3 ще бъде заменено с пълния модел на Phong.
// ============================================================

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;

uniform int       uMode;
uniform vec3      uColor;       // основен цвят / оттенък
uniform sampler2D uTexture;     // текстурен слот 0
uniform sampler2D uTexture2;    // текстурен слот 1 (скала за терена)
uniform vec3      uCameraPos;   // позиция на камерата (за мъглата)

out vec4 FragColor;

// Цвят на терена: тревата преминава в скала на стръмните места
vec3 terrainColor(vec3 n) {
    // Трева, взета в два различни мащаба и смесена -> по-малко се
    // забелязва повтарянето (tiling) на текстурата отдалеч.
    vec3 grass = mix(texture(uTexture, vUV).rgb,
                     texture(uTexture, vUV * 0.137).rgb, 0.45);
    vec3 rock  = texture(uTexture2, vUV * 0.5).rgb;

    // n.y = 1 при хоризонтална повърхност, намалява с наклона.
    // slope: 0 = равно, 1 = отвесно
    float slope = 1.0 - n.y;
    float rockAmount = smoothstep(0.18, 0.35, slope);
    return mix(grass, rock, rockAmount);
}

void main() {
    vec3 n = normalize(vNormal);

    vec3 color;
    if (uMode == 1)      color = texture(uTexture, vUV).rgb * uColor;
    else if (uMode == 2) color = terrainColor(n) * uColor;
    else                 color = uColor;

    // Временно осветяване от фиксирана посока (до Етап 3)
    vec3 lightDir = normalize(vec3(0.4, 1.0, 0.3));
    float diffuse = max(dot(n, lightDir), 0.0);
    color *= 0.35 + 0.75 * diffuse;

    // Линейна мъгла - далечните обекти плавно се сливат с небето
    vec3  skyColor = vec3(0.62, 0.78, 0.95);
    float dist = length(vWorldPos - uCameraPos);
    float fog  = clamp((dist - 120.0) / (480.0 - 120.0), 0.0, 1.0);
    color = mix(color, skyColor, fog);

    FragColor = vec4(color, 1.0);
}
