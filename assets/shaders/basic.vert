#version 330 core
// ============================================================
//  Vertex шейдър - изпълнява се ВЕДНЪЖ ЗА ВСЕКИ ВРЪХ.
//  Прилага веригата от трансформации:
//    локални -> (Model) -> световни -> (View) -> на камерата
//            -> (Projection) -> clip координати
//  и подава към fragment шейдъра позицията и нормалата в
//  СВЕТОВНИ координати - в тях се изчислява осветлението.
// ============================================================

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in mat4 aInstance;   // заема locations 3..6 - само при инстанциране

uniform mat4 uModel;
uniform mat3 uNormalMatrix;   // transpose(inverse(mat3(uModel))) - изчислена на CPU
uniform mat4 uView;
uniform mat4 uProjection;
uniform int  uInstanced;      // 1 -> рисуваме много копия (дървета)

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

void main() {
    // При инстанциране всяко копие има собствена матрица aInstance
    // (позиция, завъртане, мащаб на дървото), а uModel е общата матрица
    // на частта (ствол / корона).
    mat4 model = (uInstanced == 1) ? aInstance * uModel : uModel;

    vec4 worldPos = model * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;

    // Нормалите се трансформират с нормалната матрица, за да останат
    // перпендикулярни на повърхността при нееднакво мащабиране.
    // Обикновено тя се изчислява веднъж на обект на CPU (uNormalMatrix);
    // при инстанцирането всяко копие е различно, затова - тук.
    if (uInstanced == 1) vNormal = transpose(inverse(mat3(model))) * aNormal;
    else                 vNormal = uNormalMatrix * aNormal;
    vUV = aUV;

    gl_Position = uProjection * uView * worldPos;
}
