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

uniform mat4 uModel;
uniform mat3 uNormalMatrix;   // transpose(inverse(mat3(uModel))) - изчислена на CPU
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;

    // Нормалите се трансформират с нормалната матрица, за да останат
    // перпендикулярни на повърхността при нееднакво мащабиране.
    // Изчислява се веднъж на обект на CPU, а не за всеки връх тук.
    vNormal = uNormalMatrix * aNormal;
    vUV = aUV;

    gl_Position = uProjection * uView * worldPos;
}
