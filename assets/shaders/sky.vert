#version 330 core
// ============================================================
//  Небесна сфера - vertex шейдър.
//  Позицията на върха на единичната сфера е и ПОСОКАТА от
//  камерата към тази точка от небето.
// ============================================================

layout(location = 0) in vec3 aPos;

uniform mat4 uView;          // БЕЗ транслация - само завъртане
uniform mat4 uProjection;

out vec3 vDir;

void main() {
    vDir = aPos;
    vec4 pos = uProjection * uView * vec4(aPos, 1.0);
    // z = w -> след деленето на w дълбочината е 1.0 (най-далечната възможна)
    gl_Position = pos.xyww;
}
