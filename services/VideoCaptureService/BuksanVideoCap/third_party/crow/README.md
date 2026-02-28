# Crow (локальная копия для сборки с API)

Если не используете системный пакет Crow (например, AUR `crow`), положите сюда заголовки Crow.

## Вариант 1: Arch Linux (AUR)

```bash
yay -S crow
```

Также нужен Asio (из официальных репозиториев):

```bash
sudo pacman -S asio
```

После этого CMake найдёт Crow и Asio в системе.

## Вариант 2: Локальная копия (без AUR/сети)

1. Клонируйте репозиторий Crow (или скачайте [релиз](https://github.com/CrowCpp/Crow/releases)):

   ```bash
   git clone --depth 1 https://github.com/CrowCpp/Crow.git /tmp/Crow
   ```
   (или укажите тег, например `--branch v1.1.1`, если нужна конкретная версия)

2. Скопируйте папку `include` в этот каталог:

   ```bash
   cp -r /tmp/Crow/include third_party/crow/
   ```

   В результате должна быть структура:

   ```
   third_party/crow/include/crow.h
   third_party/crow/include/crow/...
   ```

3. Asio (Crow от него зависит) ищется в таком порядке:
   - системные пути (`/usr/include`, pacman: `sudo pacman -S asio`);
   - `third_party/asio/include`;
   - каталог `_deps/asio-src/asio/include` (если остался от старой сборки с FetchContent).

После этого соберите проект с `-DBUILD_API=ON` (по умолчанию).
