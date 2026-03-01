cmake_minimum_required(VERSION 3.16)
project(BuksanSpyNVR LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED core videoio)
find_package(yaml-cpp REQUIRED)
find_package(Threads REQUIRED)
find_package(libpqxx CONFIG QUIET)

if(TARGET libpqxx::pqxx)
  set(PQXX_TARGET libpqxx::pqxx)
elseif(TARGET pqxx::pqxx)
  set(PQXX_TARGET pqxx::pqxx)
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBPQXX IMPORTED_TARGET libpqxx)
  if(NOT LIBPQXX_FOUND)
    message(FATAL_ERROR
      "libpqxx is required for PostgreSQL metadata storage. "
      "Install package 'libpqxx' and rerun CMake.")
  endif()
  set(PQXX_TARGET PkgConfig::LIBPQXX)
endif()

set(SRC
    src/main.cpp
    src/ConfigLoader.cpp
    src/StorageManager.cpp
    src/Analytics.cpp
    src/Recorder.cpp
    src/CameraSession.cpp
    core/CameraManager.cpp
    db/IConnectionPool.cpp
    db/PostgresConnectionPool.cpp
    repositories/postgres/PostgresRecordingRepository.cpp
    repositories/postgres/PostgresCameraRepository.cpp
    repositories/postgres/PostgresNodeRepository.cpp
    services/RecordingService.cpp
    services/CameraService.cpp
    services/NodeService.cpp
    services/MetadataSyncWorker.cpp
    utils/InMemoryMetadataSyncQueue.cpp
)

add_executable(BuksanSpyNVR ${SRC})

target_include_directories(BuksanSpyNVR PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(BuksanSpyNVR PRIVATE
    ${OpenCV_LIBS}
    yaml-cpp
    ${PQXX_TARGET}
)

# ------------------------------------------------------------------------------
# REST API (optional: -DBUILD_API=ON)
# Один бинарник: с API или без. С API — управление по HTTP + загрузка из YAML.
# ------------------------------------------------------------------------------
option(BUILD_API "Build with REST API (Crow)" ON)

if(BUILD_API)
  find_package(Crow CONFIG QUIET)
  find_package(nlohmann_json CONFIG REQUIRED)

  if(NOT TARGET Crow::Crow)
    # 1) Локальная копия в third_party/crow/include
    set(CROW_THIRD_PARTY "${CMAKE_CURRENT_SOURCE_DIR}/third_party/crow/include")
    if(EXISTS "${CROW_THIRD_PARTY}/crow.h")
      set(CROW_INCLUDE_DIR "${CROW_THIRD_PARTY}")
    else()
      find_path(CROW_INCLUDE_DIR crow.h
        PATHS /usr/local/include /usr/include
        DOC "Crow include directory (или положите Crow в third_party/crow/include)")
    endif()
    if(NOT CROW_INCLUDE_DIR)
      message(FATAL_ERROR
        "Crow не найден. Варианты:\n"
        "  • Arch: yay -S crow\n"
        "  • Локально: скопируйте папку include из репозитория Crow в third_party/crow/include\n"
        "  • Или задайте CROW_INCLUDE_DIR при вызове cmake.")
    endif()

    # Crow зависит от Asio (standalone, не Boost). asio.hpp лежит в .../asio/include/
    set(ASIO_SEARCH_PATHS
      /usr/local/include
      /usr/include
      "${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/include"
      "${CMAKE_CURRENT_SOURCE_DIR}/_deps/asio-src/asio/include"
      "${CMAKE_BINARY_DIR}/_deps/asio-src/asio/include")
    # Сброс кэша, если сохранённый путь не содержит asio.hpp (например build/_deps/asio-src/include)
    if(DEFINED ASIO_INCLUDE_DIR AND NOT EXISTS "${ASIO_INCLUDE_DIR}/asio.hpp")
      unset(ASIO_INCLUDE_DIR CACHE)
    endif()
    find_path(ASIO_INCLUDE_DIR asio.hpp
      PATHS ${ASIO_SEARCH_PATHS}
      NO_DEFAULT_PATH
      DOC "Asio include directory (каталог, в котором лежит asio.hpp)")
    if(NOT ASIO_INCLUDE_DIR)
      message(FATAL_ERROR
        "Asio не найден (нужен для Crow). Варианты:\n"
        "  • Arch: pacman -S asio\n"
        "  • Локально: скопируйте include из Asio в third_party/asio/include")
    endif()

    add_library(Crow INTERFACE)
    target_include_directories(Crow INTERFACE
      ${CROW_INCLUDE_DIR}
      ${ASIO_INCLUDE_DIR})
    target_link_libraries(Crow INTERFACE Threads::Threads)
    add_library(Crow::Crow ALIAS Crow)
  endif()

  target_sources(BuksanSpyNVR PRIVATE api/HttpServer.cpp)
  target_include_directories(BuksanSpyNVR PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/api)
  target_compile_definitions(BuksanSpyNVR PRIVATE BUKSAN_BUILD_API)
  target_link_libraries(BuksanSpyNVR PRIVATE Crow::Crow nlohmann_json::nlohmann_json)
endif()

install(TARGETS BuksanSpyNVR RUNTIME DESTINATION bin)
