"""
Генератор на процедурни текстури за проекта.

Създава безшевни (tileable) текстури в assets/textures/:
    grass.png  - трева
    rock.png   - скала (за стръмните склонове на терена)
    road.png   - асфалт с бели крайни линии и прекъсната осева линия

Не е нужно да се стартира - готовите PNG файлове са включени в проекта.
Може да се замени всяка текстура с реална снимка със същото име.

Стартиране:  python tools/generate_textures.py   (изисква numpy и Pillow)
"""
import os
import numpy as np
from PIL import Image

OUT = os.path.join(os.path.dirname(__file__), "..", "assets", "textures")
rng = np.random.default_rng(42)


def tileable_noise(size, cells):
    """Безшевен шум: случайна мрежа cells x cells, разположена 3x3 пъти,
    увеличена с бикубична интерполация; взима се централното парче.
    Така левият край съвпада с десния, а горният с долния."""
    grid = rng.random((cells, cells))
    tiled = np.tile(grid, (3, 3))
    img = Image.fromarray((tiled * 255).astype(np.uint8))
    big = img.resize((size * 3, size * 3), Image.BICUBIC)
    arr = np.asarray(big, dtype=np.float32) / 255.0
    return arr[size:2 * size, size:2 * size]


def fbm(size, base_cells, octaves, persistence=0.5):
    """Fractal Brownian Motion - сума от шумове с нарастваща честота."""
    total = np.zeros((size, size), np.float32)
    amp, norm, cells = 1.0, 0.0, base_cells
    for _ in range(octaves):
        total += tileable_noise(size, cells) * amp
        norm += amp
        amp *= persistence
        cells *= 2
    return total / norm


def save(name, rgb):
    rgb = np.clip(rgb, 0, 1)
    Image.fromarray((rgb * 255).astype(np.uint8), "RGB").save(os.path.join(OUT, name))
    print("saved", name)


def grass(size=512):
    n = fbm(size, 4, 6)
    fine = rng.random((size, size)).astype(np.float32)        # ситни "стръкове"
    light = 0.75 + 0.5 * n + 0.15 * (fine - 0.5)
    dark = np.array([0.20, 0.36, 0.12])
    bright = np.array([0.42, 0.58, 0.22])
    t = fbm(size, 2, 3)[..., None]                             # петна в различни нюанси
    col = dark * (1 - t) + bright * t
    return col * light[..., None]


def rock(size=512):
    n = fbm(size, 6, 7, 0.55)
    cracks = np.abs(fbm(size, 8, 3) - 0.5) < 0.02             # тъмни пукнатини
    v = 0.35 + 0.45 * n
    v[cracks] *= 0.6
    col = np.stack([v * 1.0, v * 0.97, v * 0.92], -1)
    return col


def road(width=256, height=512):
    """u (хоризонтала) = напречно на пътя, v (вертикала) = по дължината.
    Една текстура покрива ~12 м път - половината е тире, половината празно."""
    n = np.asarray(Image.fromarray((fbm(height, 8, 6) * 255).astype(np.uint8))
                   .resize((width, height)), np.float32) / 255.0
    grain = rng.random((height, width)).astype(np.float32)
    v = 0.22 + 0.10 * n + 0.08 * (grain - 0.5)
    col = np.stack([v, v, v * 1.04], -1)

    u = (np.arange(width) + 0.5) / width
    vv = (np.arange(height) + 0.5) / height
    U, V = np.meshgrid(u, vv)

    paint = np.array([0.92, 0.92, 0.90])
    edge = ((U > 0.035) & (U < 0.065)) | ((U > 0.935) & (U < 0.965))   # крайни линии
    center = (np.abs(U - 0.5) < 0.015) & (V < 0.5)                      # прекъсната ос
    mask = (edge | center)[..., None]
    worn = 0.85 + 0.15 * grain[..., None]                               # леко износена боя
    return np.where(mask, paint * worn, col)


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    save("grass.png", grass())
    save("rock.png", rock())
    save("road.png", road())
