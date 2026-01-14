cmake_minimum_required(VERSION 3.28)
project(cubeRebuild LANGUAGES C CXX)

# --- 1. Настройки проекта ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

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

        # LLD - скорость
        add_link_options("-fuse-ld=lld")

        # Mimalloc - скорость
        set(USE_MIMALLOC ON)
endif()

# --- 3. Загрузка зависимостей (FetchContent) ---
include(FetchContent)

# === GLFW ===
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
# Строго X11 для Linux
if(UNIX AND NOT WIN32)
        set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
        set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
endif()
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
FetchContent_MakeAvailable(glfw)

# === GLM ===
set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1)
FetchContent_MakeAvailable(glm)

# === XSIMD ===
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(xsimd GIT_REPOSITORY https://github.com/xtensor-stack/xsimd.git GIT_TAG 13.0.0)
FetchContent_MakeAvailable(xsimd)

# === MIMALLOC (Только Linux) ===
if(USE_MIMALLOC)
        set(MIMALLOC_INTEL_ATOMIC ON CACHE BOOL "" FORCE)
        set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
        set(MI_BUILD_OBJECT OFF CACHE BOOL "" FORCE)
        set(MI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(MI_OVERRIDE OFF CACHE BOOL "" FORCE)

        FetchContent_Declare(mimalloc GIT_REPOSITORY https://github.com/microsoft/mimalloc GIT_TAG v2.1.2)
        FetchContent_MakeAvailable(mimalloc)
endif()

# === STB IMAGE (НОВОЕ: Исправление ошибки линковки) ===
FetchContent_Declare(stb GIT_REPOSITORY https://github.com/nothings/stb.git GIT_TAG master)
FetchContent_MakeAvailable(stb)

# Создаем библиотеку для компиляции реализации stb_image
# Генерируем файл .c, который включает заголовок с дефайном
set(STB_IMPL_FILE "${CMAKE_BINARY_DIR}/stb_image_impl.c")
if(NOT EXISTS "${STB_IMPL_FILE}")
        file(WRITE "${STB_IMPL_FILE}" "#define STB_IMAGE_IMPLEMENTATION\n#include \"stb_image.h\"\n")
endif()

add_library(stb_lib STATIC "${STB_IMPL_FILE}")
target_include_directories(stb_lib PUBLIC "${stb_SOURCE_DIR}")
# =======================================================

# === FASTNOISE 2 ===
cmake_policy(SET CMP0048 NEW)
set(FASTNOISE2_NOISETOOL OFF CACHE BOOL "" FORCE)
set(FASTNOISE2_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(fastnoise2 GIT_REPOSITORY https://github.com/Auburn/FastNoise2.git GIT_TAG v0.10.0-alpha)
FetchContent_MakeAvailable(fastnoise2)

# === GLAD ===
FetchContent_Declare(glad GIT_REPOSITORY https://github.com/Dav1dde/glad.git GIT_TAG v0.1.36)
set(GLAD_PROFILE "core" CACHE STRING "OpenGL profile" FORCE)
set(GLAD_API "gl=4.6" CACHE STRING "OpenGL API version" FORCE)
set(GLAD_GENERATOR "c" CACHE STRING "Language generator" FORCE)
set(GLAD_SPEC "gl" CACHE STRING "Specification" FORCE)
FetchContent_MakeAvailable(glad)

# --- 4. Исполняемый файл ---
add_executable(cubeRebuild main.cpp
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
set(RELEASE_FLAGS -O3 -funroll-loops -ftree-vectorize -march=native -ffast-math -flto -fno-plt -Wno-nan-infinity-disabled)

target_compile_options(cubeRebuild PRIVATE $<$<CONFIG:Release>:${RELEASE_FLAGS}>)
target_link_options(cubeRebuild PRIVATE $<$<CONFIG:Release>:-flto -O3 -fno-plt>)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(cubeRebuild PRIVATE -O1 -g -fno-omit-frame-pointer)
endif()

# --- 6. Линковка ---
target_link_libraries(cubeRebuild PRIVATE
        glfw
        ${OPENGL_LIB}
        glm::glm
        xsimd
        glad
        FastNoise
        stb_lib     # <-- Добавили нашу новую библиотеку для картинок
)

if(USE_MIMALLOC)
        target_link_libraries(cubeRebuild PRIVATE mimalloc-static)
        target_include_directories(cubeRebuild PRIVATE ${mimalloc_SOURCE_DIR}/include)
endif()

if(UNIX AND NOT WIN32)
        target_link_libraries(cubeRebuild PRIVATE pthread dl)
endif()

file(COPY ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_SOURCE_DIR}/textures DESTINATION ${CMAKE_BINARY_DIR})