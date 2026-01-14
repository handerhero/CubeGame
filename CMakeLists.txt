cmake_minimum_required(VERSION 3.28)
project(cubeRebuild LANGUAGES C CXX)

# --- 1. Настройки проекта ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# !!! УДАЛЕНО: Блок с libc++.
# Используем дефолтный libstdc++, чтобы не было конфликтов ABI с системой и драйвером.

# --- 2. Логика OS ---
if (WIN32)
        message(STATUS "Configuring for Windows (Cross-Compile)")
        set(OPENGL_LIB opengl32)
        add_link_options(-static)
        set(USE_MIMALLOC OFF)
else()
        message(STATUS "Configuring for Linux")
        find_package(OpenGL REQUIRED)
        set(OPENGL_LIB OpenGL::GL)

        # LLD - оставляем для скорости сборки
        add_link_options("-fuse-ld=lld")

        # Mimalloc - оставляем, он дает буст
        set(USE_MIMALLOC ON)
endif()

# --- 3. Загрузка зависимостей ---
include(FetchContent)

# GLFW (Строго под X11 для скорости)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)     # X11 On
set(GLFW_BUILD_WAYLAND ON CACHE BOOL "" FORCE) # Wayland Off
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
FetchContent_MakeAvailable(glfw)

# GLM, XSIMD
set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1)
FetchContent_MakeAvailable(glm)

set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(xsimd GIT_REPOSITORY https://github.com/xtensor-stack/xsimd.git GIT_TAG 13.0.0)
FetchContent_MakeAvailable(xsimd)

# MIMALLOC (Исправленная конфигурация)
if(USE_MIMALLOC)
        set(MIMALLOC_INTEL_ATOMIC ON CACHE BOOL "" FORCE)
        set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
        set(MI_BUILD_OBJECT OFF CACHE BOOL "" FORCE)
        set(MI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        # Отключаем авто-перехват в CMake, сделаем это хедером в коде
        set(MI_OVERRIDE OFF CACHE BOOL "" FORCE)

        FetchContent_Declare(mimalloc GIT_REPOSITORY https://github.com/microsoft/mimalloc GIT_TAG v2.1.2)
        FetchContent_MakeAvailable(mimalloc)
endif()

# FastNoise2 & GLAD
cmake_policy(SET CMP0048 NEW)
set(FASTNOISE2_NOISETOOL OFF CACHE BOOL "" FORCE)
set(FASTNOISE2_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(Libs/FastNoise2-master)

add_library(glad STATIC Libs/glad/src/glad.c Libs/glad/src/stb.cpp)
target_include_directories(glad PUBLIC Libs/glad/include Definitions/Libs)

# --- 4. Исполняемый файл ---
add_executable(cubeRebuild main.cpp
        # ... ВАШИ ФАЙЛЫ ...
        Source/IOReactions/Mouse.cpp
        Source/Render/GpuManager.cpp
        Source/Debug/GLDebug.cpp
        Source/ChunkSystem/ChunkGenerationSystem.cpp
        Source/Configuration/Window.cpp
        Source/Camera/Camera.cpp
        Source/ObjectsAndPhysic/Player.cpp
        Source/ObjectsAndPhysic/AABB.cpp
        Source/ChunkSystem/Chunk.cpp
        Source/ChunkSystem/ChunkAllocator.cpp
        Source/ObjectsAndPhysic/Physic.cpp
        Source/IOReactions/Keyboard.cpp
        Source/Math/MathUtils.cpp
        Source/IOReactions/Callbacks.cpp
        Source/Utils/VRamAllocator.cpp
        Source/Render/CreateShader.cpp
)

target_sources(cubeRebuild PUBLIC
        FILE_SET CXX_MODULES FILES
        # ... ВАШИ МОДУЛИ ...
        Definitions/Core/Chunk.cppm
        Definitions/Core/ChunkAllocator.cppm
        Definitions/RenderEngine/Camera.cppm
        Definitions/Core/ChunkGenerationSystem.cppm
        Definitions/Core/Player.cppm
        Definitions/PhysicEngine/Physic.cppm
        Definitions/PhysicEngine/Keyboard.cppm
        Definitions/PhysicEngine/AABB.cppm
        Definitions/Libs/MathUtils.cppm
        Definitions/Platform/Callbacks.cppm
        Definitions/PhysicEngine/Mouse.cppm
        Definitions/Libs/HashMapMod.cppm
        Definitions/RenderEngine/VRamAllocator.cppm
        Definitions/RenderEngine/GLDebug.cppm
        Definitions/RenderEngine/FPSCounter.cppm
        Definitions/Platform/Window.cppm
        Definitions/RenderEngine/CreateShader.cppm
        Definitions/RenderEngine/GpuManager.cppm
        Definitions/PhysicEngine/PhysicEngine.cppm
        Definitions/RenderEngine/RenderEngine.cppm
        Definitions/Core/Core.cppm
        Definitions/Platform/IOData.cppm
        Definitions/Libs/Frustum.cppm
)

target_include_directories(cubeRebuild PRIVATE Definitions Definitions/)

# --- 5. Флаги и Оптимизация ---
# Вернули стандартные флаги. -march=native обязателен.
set(RELEASE_FLAGS -O3 -funroll-loops -ftree-vectorize -march=native -ffast-math -flto -fno-plt -Wno-nan-infinity-disabled)

target_compile_options(cubeRebuild PRIVATE $<$<CONFIG:Release>:${RELEASE_FLAGS}>)
target_link_options(cubeRebuild PRIVATE $<$<CONFIG:Release>:-flto -O3 -fno-plt>)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(cubeRebuild PRIVATE -O1 -g -fno-omit-frame-pointer)
        if (UNIX AND NOT WIN32)
                target_compile_options(cubeRebuild PRIVATE -fsanitize=address,undefined)
                target_link_options(cubeRebuild PRIVATE -fsanitize=address,undefined)
        endif()
endif()

# --- 6. Линковка ---
target_link_libraries(cubeRebuild PRIVATE
        glfw
        ${OPENGL_LIB}
        glm::glm
        xsimd
        glad
        FastNoise
)

if(USE_MIMALLOC)
        # Линкуем статическую библиотеку.
        target_link_libraries(cubeRebuild PRIVATE mimalloc-static)
        # Добавляем путь к хедерам (чтобы в main.cpp работал include)
        target_include_directories(cubeRebuild PRIVATE ${mimalloc_SOURCE_DIR}/include)
endif()

if(UNIX AND NOT WIN32)
        # Обязательно pthread и dl для Linux
        target_link_libraries(cubeRebuild PRIVATE pthread dl)
endif()

file(COPY ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_SOURCE_DIR}/textures DESTINATION ${CMAKE_BINARY_DIR})