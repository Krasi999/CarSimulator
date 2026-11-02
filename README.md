# Интерактивен 3D автомобилен симулатор (OpenGL 3.3 Core)

Курсов проект по BCSE51 Компютърна графика, ТУ – София.

## Технологии
| Компонент | Роля |
|---|---|
| C++17 | език за програмиране |
| OpenGL 3.3 Core + GLSL 330 | графичен API, шейдъри |
| GLFW 3.4 | прозорец, контекст, вход от клавиатура/мишка |
| GLAD | зареждане на OpenGL функциите |
| GLM 1.0.1 | вектори, матрици, трансформации |
| stb_image | зареждане на PNG/JPG текстури (external/stb) |
| CMake 3.16+ | система за компилиране |

GLFW и GLM се изтеглят **автоматично** от CMake при първото конфигуриране (нужен е интернет).
GLAD вече е включен в `external/glad`.

## Структура
```
CarSimulator/
├── CMakeLists.txt
├── external/glad/        OpenGL loader (генериран за GL 3.3 Core)
├── external/stb/         stb_image.h - зареждане на изображения
├── assets/shaders/       GLSL шейдъри (.vert, .frag)
├── assets/textures/      текстури (трева, скала, път)
├── tools/                generate_textures.py - процедурен генератор на текстурите
└── src/
    ├── main.cpp          прозорец, вход, главен цикъл, сцена
    ├── Shader.h/.cpp     компилиране на шейдъри, uniform-и
    ├── Camera.h/.cpp     свободна камера (yaw/pitch, lookAt, perspective)
    ├── Mesh.h/.cpp       VAO/VBO/EBO, примитиви (куб, равнина)
    ├── Texture.h/.cpp    текстури, mipmaps, филтриране
    ├── Road.h/.cpp       писта по Catmull-Rom сплайн
    └── Terrain.h/.cpp    терен от height map (fBm шум)
```

## Управление
| Клавиш | Действие |
|---|---|
| W / A / S / D | движение на камерата |
| Space / Ctrl | нагоре / надолу |
| Shift | по-бързо |
| Мишка / колелце | оглеждане / zoom |
| F1 | каркасен режим (wireframe) |
| Tab | освобождава курсора |
| Esc | изход |

## Настройка на средата – Windows (препоръчително)
1. Инсталирай **Visual Studio 2022 Community**: https://visualstudio.microsoft.com/
   В инсталатора избери workload **„Desktop development with C++“**
   (той включва компилатора MSVC и CMake – нищо друго не е нужно).
2. Инсталирай **Git**: https://git-scm.com/download/win (CMake го ползва, за да изтегли GLFW и GLM).
3. Разархивирай проекта, напр. в `C:\Projects\CarSimulator`
   (по-добре път **без кирилица и интервали**).
4. Visual Studio → **File → Open → Folder…** → избери папката `CarSimulator`.
   VS ще открие `CMakeLists.txt` и сам ще конфигурира проекта (1–2 мин. първия път).
5. Горе в лентата избери **Startup Item: CarSimulator.exe** и натисни **F5**.

## Алтернатива – VS Code
Инсталирай Visual Studio Build Tools (C++), CMake и Git, а във VS Code – разширенията
*C/C++* и *CMake Tools*. Отвори папката → `CMake: Configure` → `CMake: Build` → Run.

## Команден ред (Windows / Linux / macOS)
```bash
cmake -S . -B build
cmake --build build --config Release
# Windows: build\Release\CarSimulator.exe    Linux/macOS: ./build/CarSimulator
```
Linux допълнително изисква: `sudo apt install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl-dev`

## Проверка
Отваря се прозорец с хълмист терен и асфалтова писта; камерата е в началото на пистата.
В конзолата се отпечатва версията на OpenGL и видеокартата (трябва да е 3.3 или по-нова).
`ESC` затваря прозореца.

## Чести проблеми
- **„Не може да се отвори файл: …shaders/basic.vert“** или **„Не може да се зареди текстура“** – проектът е преместен след конфигуриране.
  Изтрий папката `out/` (VS) или `build/` и конфигурирай отново.
- **„Window creation failed“** – драйверите на видеокартата са стари; обнови ги
  (Intel/NVIDIA/AMD). Поддръжка на OpenGL 3.3 има всяка карта след ~2010 г.
- **Грешка при изтегляне на GLFW/GLM** – провери интернета и че Git е инсталиран.
