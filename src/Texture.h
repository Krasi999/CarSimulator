#pragma once
// ============================================================
//  Texture - 2D текстура, заредена от файл (PNG/JPG) със stb_image
//  и качена във видеопаметта.
//
//  Настройки:
//    - GL_REPEAT        -> текстурата се повтаря (UV > 1 е позволено)
//    - Mipmaps          -> намалени копия за далечни повърхности
//    - Трилинейно филтриране + анизотропно филтриране (ако се поддържа)
// ============================================================
#include <glad/glad.h>
#include <string>

class Texture {
public:
    explicit Texture(const std::string& path);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Свързва текстурата към текстурен слот (unit) 0, 1, 2...
    // Шейдърът чете от слота чрез uniform sampler2D със стойност = номера на слота.
    void bind(unsigned int unit) const;

    int width()  const { return m_width; }
    int height() const { return m_height; }

private:
    GLuint m_id = 0;
    int m_width = 0, m_height = 0;
};
