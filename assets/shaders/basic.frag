#version 330 core
// ============================================================
//  Fragment шейдър - изпълнява се ЗА ВСЕКИ ПИКСЕЛ (фрагмент).
//
//  Етап 3: Модел на осветяване на Phong (per-pixel / Phong shading)
//
//    I = Ka*Ia  +  Σ светлини [ Kd*max(N·L,0)*Id  +  Ks*max(R·V,0)^n*Is ] * затихване
//
//  N - нормала, L - посока към светлината, V - посока към камерата,
//  R - отразеният лъч на L спрямо N, n - shininess (степен на блясък).
//
//  Цвят на повърхността:
//    uMode = 0 -> само цвят на материала
//    uMode = 1 -> текстура x цвят
//    uMode = 2 -> терен: трева + скала, смесени според наклона
// ============================================================

#define MAX_POINT_LIGHTS 16

struct Material {
    vec3  color;
    float specular;     // Ks
    float shininess;    // n
    vec3  emissive;     // собствено светене
};

struct DirLight {
    vec3 direction;     // посока КЪМ светлината (нормализирана)
    vec3 color;         // Id = Is
    vec3 ambient;       // Ia
};

struct PointLight {
    vec3  position;
    vec3  color;
    float constant;
    float linear;
    float quadratic;
};

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;

uniform int        uMode;
uniform Material   uMaterial;
uniform DirLight   uSun;
uniform PointLight uPointLights[MAX_POINT_LIGHTS];
uniform int        uNumPointLights;

uniform vec3 uComponents;   // (ambient, diffuse, specular): 1 = вкл., 0 = изкл.
uniform int  uBlinn;        // 0 = Phong, 1 = Blinn-Phong

uniform sampler2D uTexture;     // текстурен слот 0
uniform sampler2D uTexture2;    // текстурен слот 1 (скала за терена)
uniform vec3      uCameraPos;

out vec4 FragColor;

// ------------------------------------------------------------
//  Цвят на терена: тревата преминава в скала на стръмните места
// ------------------------------------------------------------
vec3 terrainColor(vec3 n) {
    vec3 grass = mix(texture(uTexture, vUV).rgb,
                     texture(uTexture, vUV * 0.137).rgb, 0.45);
    vec3 rock  = texture(uTexture2, vUV * 0.5).rgb;
    float slope = 1.0 - n.y;
    return mix(grass, rock, smoothstep(0.18, 0.35, slope));
}

// ------------------------------------------------------------
//  Дифузна + огледална част за ЕДНА светлина.
//  L - единичен вектор от повърхността КЪМ светлината.
// ------------------------------------------------------------
vec3 shade(vec3 N, vec3 V, vec3 L, vec3 lightColor, vec3 albedo) {
    // Дифузна компонента (закон на Ламберт): cos на ъгъла между N и L
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = NdotL * lightColor * albedo;

    // Огледална компонента
    float spec = 0.0;
    if (NdotL > 0.0) {              // светлина зад повърхността не дава блясък
        if (uBlinn == 1) {
            // Blinn-Phong: half-вектор между L и V
            vec3 H = normalize(L + V);
            spec = pow(max(dot(N, H), 0.0), uMaterial.shininess * 4.0);
        } else {
            // Класически Phong: отразен лъч R
            vec3 R = reflect(-L, N);
            spec = pow(max(dot(R, V), 0.0), uMaterial.shininess);
        }
    }
    // Блясъкът е с цвета на светлината, не на повърхността
    vec3 specular = uMaterial.specular * spec * lightColor;

    return diffuse * uComponents.y + specular * uComponents.z;
}

void main() {
    vec3 N = normalize(vNormal);                      // след интерполация вече не е единичен
    vec3 V = normalize(uCameraPos - vWorldPos);       // към наблюдателя

    // ---- Основен цвят (albedo) на повърхността ----
    vec3 albedo;
    if (uMode == 1)      albedo = texture(uTexture, vUV).rgb * uMaterial.color;
    else if (uMode == 2) albedo = terrainColor(N) * uMaterial.color;
    else                 albedo = uMaterial.color;

    // ---- Ambient ----
    vec3 result = uSun.ambient * albedo * uComponents.x;

    // ---- Слънце (насочена светлина) ----
    result += shade(N, V, normalize(uSun.direction), uSun.color, albedo);

    // ---- Точкови светлини (лампи) ----
    for (int i = 0; i < uNumPointLights; ++i) {
        vec3  toLight = uPointLights[i].position - vWorldPos;
        float d = length(toLight);
        vec3  L = toLight / d;

        // Затихване: 1 / (Kc + Kl*d + Kq*d^2)
        float attenuation = 1.0 / (uPointLights[i].constant
                                 + uPointLights[i].linear    * d
                                 + uPointLights[i].quadratic * d * d);

        result += shade(N, V, L, uPointLights[i].color, albedo) * attenuation;
    }

    // ---- Собствено светене (не зависи от осветлението) ----
    result += uMaterial.emissive;

    // ---- Мъгла ----
    vec3  skyColor = vec3(0.62, 0.78, 0.95);
    float dist = length(vWorldPos - uCameraPos);
    float fog  = clamp((dist - 120.0) / (480.0 - 120.0), 0.0, 1.0);
    result = mix(result, skyColor, fog);

    FragColor = vec4(result, 1.0);
}
