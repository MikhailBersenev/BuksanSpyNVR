# BuksanVideoCap: сборка и запуск

Сервис записи RTSP-сегментов с REST API и хранением метаданных в PostgreSQL.

## 1) Требования

Минимально нужны:

- C++17 компилятор (`g++`/`clang++`)
- `cmake >= 3.16`
- `ninja` (рекомендуется) или `make`
- OpenCV (модули `core`, `videoio`)
- `yaml-cpp`
- `nlohmann-json`
- `asio` (для Crow)
- `crow` (или `third_party/crow/include/crow.h`)
- `libpqxx` (обязателен для PostgreSQL-слоя)

Для Arch Linux пример установки:

```bash
sudo pacman -S --needed cmake ninja gcc opencv yaml-cpp nlohmann-json asio libpqxx postgresql
```

Если системного пакета Crow нет, используйте локальную копию в `third_party/crow/include`.

## 2) Подготовка PostgreSQL

Запустите PostgreSQL, создайте БД и примените схему:

```bash
sudo -iu postgres psql -c "CREATE DATABASE buksanspy;"
psql "dbname=buksanspy user=postgres host=127.0.0.1 port=5432" -f db/schema.sql
```

По умолчанию сервис использует DSN:

`dbname=buksanspy user=postgres password=postgres host=127.0.0.1 port=5432`

## 3) Конфигурация сервиса

Проверьте `config.yaml`:

```yaml
storage_path: /path/to/storage

cameras:
  - id: cam1
    rtsp_url: rtsp://user:pass@camera/Streaming/Channels/101
    record: true
    analytics: false
```

`storage_path` должен существовать или быть доступным для создания.

## 4) Сборка

Из директории `services/VideoCaptureService/BuksanVideoCap`:

```bash
cmake -S . -B build -G Ninja -DBUILD_API=ON
cmake --build build -j
```

Бинарник: `build/BuksanSpyNVR`

## 5) Запуск

### Базовый запуск

```bash
./build/BuksanSpyNVR --config config.yaml --api 8080
```

### Запуск без API

```bash
./build/BuksanSpyNVR --config config.yaml --no-api
```

### Переменные окружения PostgreSQL/синхронизации

- `BUKSAN_PG_DSN` — строка подключения к PostgreSQL
- `BUKSAN_PG_POOL_SIZE` — размер пула подключений (по умолчанию `8`)
- `BUKSAN_METADATA_RETRY_SECONDS` — период retry flush очереди (по умолчанию `2`)
- `BUKSAN_METADATA_RETRY_BATCH` — размер batch при flush (по умолчанию `64`)

Пример:

```bash
export BUKSAN_PG_DSN="dbname=buksanspy user=postgres password=postgres host=127.0.0.1 port=5432"
export BUKSAN_PG_POOL_SIZE=10
export BUKSAN_METADATA_RETRY_SECONDS=2
export BUKSAN_METADATA_RETRY_BATCH=100
./build/BuksanSpyNVR --config config.yaml --api 8080
```

## 6) API (основные маршруты)

### Health

- `GET /api/v1/health`

### Камеры (текущий API управления)

- `GET /api/v1/cameras`
- `GET /api/v1/cameras/{id}`
- `POST /api/v1/cameras`
- `POST /api/v1/cameras/{id}/start`
- `POST /api/v1/cameras/{id}/stop`
- `DELETE /api/v1/cameras/{id}`

### Узлы

- `GET /api/v1/nodes`

### Записи

- `GET /recordings?camera_id={id}&from={unix_from}&to={unix_to}`
- `GET /recordings/{id}`
- `POST /recordings`к
- `GET /recordings/{id}/stream` (поддержка `Range`, chunked-streaming)

## 7) Быстрая проверка

```bash
curl -s http://127.0.0.1:8080/api/v1/health
curl -s "http://127.0.0.1:8080/recordings?camera_id=1&from=1700000000&to=1800000000"
curl -v -H "Range: bytes=0-1023" http://127.0.0.1:8080/recordings/1/stream -o /tmp/chunk.bin
```

## 8) Поведение при недоступной БД

Если PostgreSQL временно недоступен:

- запись видео на диск продолжается;
- метаданные сегментов ставятся в in-memory очередь;
- фоновый воркер повторно отправляет метаданные в БД после восстановления подключения.

## 9) Типовые проблемы

- `libpqxx is required...`  
  Установите `libpqxx`, затем заново выполните `cmake -S . -B build`.

- `Crow не найден`  
  Установите Crow системно или положите `crow.h` в `third_party/crow/include`.

- `Asio не найден`  
  Установите пакет `asio` или положите Asio include в `third_party/asio/include`.
