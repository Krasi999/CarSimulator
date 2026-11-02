#include "Texture.h"

// stb_image е "single-header" библиотека: декларациите са в .h файла,
// а имплементацията се включва точно в ЕДИН .cpp файл чрез този макрос.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstring>
#include <stdexcept>

// Анизотропното филтриране е разширение (GL_EXT_texture_filter_anisotropic),
// поддържано от практически всички видеокарти, но не е част от ядрото на 3.3.
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

static bool hasExtension(const char* name) {
    GLint count = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &count);
    for (GLint i = 0; i < count; ++i) {
        const char* ext = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
        if (ext && std::strcmp(ext, name) == 0) return true;
    }
    return false;
}

Texture::Texture(const std::string& path) {
    // Във файла изображението е записано отгоре надолу, а OpenGL очаква
    // първият ред да е ДОЛНИЯТ (v = 0 е долу). Затова обръщаме по вертикала.
    stbi_set_flip_vertically_on_load(true);

    int channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &channels, 4);
    if (!data) throw std::runtime_error("Не може да се зареди текстура: " + path);

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    // Качване на пикселите (винаги ги искаме като RGBA - последният аргумент на stbi_load)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);   // копието в RAM вече не е нужно - данните са в GPU

    // Mipmap нива: 512x512 -> 256x256 -> ... -> 1x1
    glGenerateMipmap(GL_TEXTURE_2D);

    // Повтаряне извън [0,1]
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Намаляване: трилинейно (линейно в mipmap нивото + линейно между нивата)
    // Увеличаване: билинейно
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Анизотропно филтриране - пътят остава рязък, когато го гледаме под остър ъгъл
    if (hasExtension("GL_EXT_texture_filter_anisotropic") ||
        hasExtension("GL_ARB_texture_filter_anisotropic")) {
        GLfloat maxAniso = 1.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                        maxAniso < 8.0f ? maxAniso : 8.0f);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::~Texture() { glDeleteTextures(1, &m_id); }

void Texture::bind(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_id);
}
