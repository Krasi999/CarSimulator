#version 330 core
// Тестов fragment шейдър: оцветява всеки пиксел с интерполирания цвят

in vec3 vColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vColor, 1.0);
}
