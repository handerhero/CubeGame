cmake_minimum_required(VERSION 3.24)
project(cubeRebuild LANGUAGES C CXX)

# ------------------------------------------------------------
# 1. НАСТРОЙКИ ПРОЕКТА И ЯЗЫКА
# ------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Настройка для FastNoise2 (политика должна быть вверху)
cmake_policy(SET CMP0048 NEW)

# ------------------------------------------------------------
# 2. ЗАГРУЗКА ЗАВИСИМОСТЕЙ (FetchContent)
# ------------------------------------------------------------
include(FetchContent)

# --- GLFW ---
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

# --- GLM ---
set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# --- XSIMD ---
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_BENCHMARK OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        xsimd
        GIT_REPOSITORY https://github.com/xtensor-stack/xsimd.git
        GIT_TAG 13.0.0
)
FetchContent_MakeAvailable(xsimd)

# --- ЛОКАЛЬНЫЕ ЗАВИСИМОСТИ ---
find_package(OpenGL REQUIRED)

# FastNoise2
set(FASTNOISE2_NOISETOOL OFF CACHE BOOL "" FORCE)
set(FASTNOISE2_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(Libs/FastNoise2-master)

# GLAD + STB (Собираем как статическую библиотеку)
add_library(glad STATIC
        Libs/glad/src/glad.c
        Libs/glad/src/stb.cpp
)
target_include_directories(glad PUBLIC
        Libs/glad/include
        Headers/Libs
)

# ------------------------------------------------------------
# 3. ИСПОЛНЯЕМЫЙ ФАЙЛ
# ------------------------------------------------------------
add_executable(cubeRebuild
        main.cpp
        Source/IOReactions/MouseInteractions.cpp
        Source/Render/ChunksGpuManager.cpp
        Source/Debug/GLDebug.cpp
        Source/ChunkSystem/ChunkGenerationManager.cpp
        Source/Configuration/Window.cpp
        Source/Camera/Camera.cpp
        Source/ObjectsAndPhysic/Player.cpp
        Source/ObjectsAndPhysic/AABB.cpp
        Source/ChunkSystem/Chunk.cpp

        # Хедеры добавлять в sources не обязательно, но полезно для IDE
        Headers/Camera/Camera.hpp
        Headers/Render/Window.hpp
        Headers/ChunkSystem/ChunkAllocator.hpp
        Headers/ChunkSystem/HashSystem.hpp
)

target_include_directories(cubeRebuild PRIVATE
        Headers
        Headers/Libs
)

# ------------------------------------------------------------
# 4. ФЛАГИ КОМПИЛЯЦИИ И ОПТИМИЗАЦИИ
# ------------------------------------------------------------

# Общие флаги оптимизации для Clang/GCC
target_compile_options(cubeRebuild PRIVATE
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-unroll-loops>
        $<$<CONFIG:Release>:-ftree-vectorize>
        # Можно добавить архитектуру, но АККУРАТНО.
        # -march=x86-64-v3 - безопасно для современных CPU (c 2015 года)
        # -march=native - НЕЛЬЗЯ при кросс-компиляции
        $<$<CONFIG:Release>:-march=x86-64-v3>
)

# ------------------------------------------------------------
# 5. ОТЛАДКА И САНИТАЙЗЕРЫ
# ------------------------------------------------------------
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        # Базовые флаги отладки
        target_compile_options(cubeRebuild PRIVATE
                -O1
                -g
                -fno-omit-frame-pointer
                -fno-optimize-sibling-calls
                -fno-inline
                -D_GLIBCXX_ASSERTIONS # Включает проверки bounds check в vector/string
        )

        # Список санитайзеров
        set(SAN_FLAGS "-fsanitize=address" "-fsanitize=undefined")

        # LeakSanitizer обычно плохо работает на Windows/MinGW, включаем только на Linux
        if (UNIX AND NOT MINGW)
                list(APPEND SAN_FLAGS "-fsanitize=leak")
        endif()

        target_compile_options(cubeRebuild PRIVATE ${SAN_FLAGS})
        target_link_options(cubeRebuild PRIVATE ${SAN_FLAGS})
endif()

# ------------------------------------------------------------
# 6. ЛИНКОВКА
# ------------------------------------------------------------
target_link_libraries(cubeRebuild PRIVATE
        glfw
        OpenGL::GL
        glm::glm
        xsimd
        glad
        FastNoise
)

# Статическая лилинковка уже задана в Toolchain-файле (-static),
# дублировать здесь не нужно.

# ------------------------------------------------------------
# 7. КОПИРОВАНИЕ РЕСУРСОВ
# ------------------------------------------------------------
file(COPY ${CMAKE_SOURCE_DIR}/shaders  DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/textures DESTINATION ${CMAKE_BINARY_DIR})