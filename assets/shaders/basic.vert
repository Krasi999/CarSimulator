#version 330 core
// ============================================================
//  Vertex шейдър - изпълнява се ВЕДНЪЖ ЗА ВСЕКИ ВРЪХ.
//  Прилага веригата от трансформации:
//    локални -> (Model) -> световни -> (View) -> на камерата
//            -> (Projection) -> clip координати
// ============================================================

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vWorldPos;   // позиция в света - към fragment шейдъра
out vec3 vNormal;     // нормала в света
out vec2 vUV;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;

    // Нормалите се трансформират с обратно-транспонираната матрица,
    // за да останат перпендикулярни на повърхността при
    // нееднакво мащабиране (напр. куб, разтеглен по X).
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    vUV = aUV;

    gl_Position = uProjection * uView * worldPos;
}
