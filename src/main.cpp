// ============================================================
//  Етап 0: Тест на средата за разработка
//  Ако виждаш въртящ се цветен куб - GLFW, GLAD, GLM и
//  шейдърите работят и можем да продължим към симулатора.
// ============================================================
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "Shader.h"

static int g_width = 1280, g_height = 720;

static void framebufferSizeCallback(GLFWwindow*, int w, int h) {
    g_width = w; g_height = h;
    glViewport(0, 0, w, h);
}

int main() {
    // ---- 1. Инициализация на GLFW и създаване на прозорец ----
    if (!glfwInit()) { std::cerr << "GLFW init failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(g_width, g_height,
                                          "Car Simulator - test", nullptr, nullptr);
    if (!window) { std::cerr << "Window creation failed\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // V-Sync
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // ---- 2. Зареждане на OpenGL функциите чрез GLAD ----
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD failed\n"; return -1;
    }
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n"
              << "GPU:    " << glGetString(GL_RENDERER) << "\n";

    glfwGetFramebufferSize(window, &g_width, &g_height);
    glViewport(0, 0, g_width, g_height);
    glEnable(GL_DEPTH_TEST);

    try {
        Shader shader(ASSET_DIR "shaders/test.vert", ASSET_DIR "shaders/test.frag");

        // ---- 3. Геометрия на куб: 8 върха (позиция + цвят) и 36 индекса ----
        float vertices[] = {
            -0.5f,-0.5f,-0.5f, 1,0,0,   0.5f,-0.5f,-0.5f, 0,1,0,
             0.5f, 0.5f,-0.5f, 0,0,1,  -0.5f, 0.5f,-0.5f, 1,1,0,
            -0.5f,-0.5f, 0.5f, 1,0,1,   0.5f,-0.5f, 0.5f, 0,1,1,
             0.5f, 0.5f, 0.5f, 1,1,1,  -0.5f, 0.5f, 0.5f, 0.2f,0.2f,0.2f };
        unsigned int indices[] = {
            0,1,2, 2,3,0,  4,5,6, 6,7,4,  0,4,7, 7,3,0,
            1,5,6, 6,2,1,  3,2,6, 6,7,3,  0,1,5, 5,4,0 };

        GLuint vao, vbo, ebo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);

        // ---- 4. Главен цикъл ----
        while (!glfwWindowShouldClose(window)) {
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, true);

            glClearColor(0.45f, 0.70f, 0.95f, 1.0f); // небесносиньо
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            float t = (float)glfwGetTime();
            float aspect = g_height > 0 ? (float)g_width / g_height : 1.0f;

            glm::mat4 model = glm::rotate(glm::mat4(1.0f), t, glm::vec3(0.5f, 1.0f, 0.0f));
            glm::mat4 view  = glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0), glm::vec3(0, 1, 0));
            glm::mat4 proj  = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);

            shader.use();
            shader.setMat4("uModel", model);
            shader.setMat4("uView", view);
            shader.setMat4("uProjection", proj);

            glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        glfwTerminate();
        return -1;
    }

    glfwTerminate();
    return 0;
}
