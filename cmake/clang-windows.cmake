set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# === ПУТЬ К УСТАНОВЛЕННОМУ ПАКЕТУ ИЗ AUR ===
set(TOOLCHAIN_PATH "/home/aspadien/CLionProjects/cubeRebuild/CrossCompiler/llvm-mingw-20251216-ucrt-ubuntu-22.04-x86_64")
# ===========================================

# Указываем конкретные компиляторы из пакета
set(CMAKE_C_COMPILER "/home/aspadien/CLionProjects/cubeRebuild/CrossCompiler/llvm-mingw-20251216-ucrt-ubuntu-22.04-x86_64/bin/x86_64-w64-mingw32-clang")
set(CMAKE_CXX_COMPILER "/home/aspadien/CLionProjects/cubeRebuild/CrossCompiler/llvm-mingw-20251216-ucrt-ubuntu-22.04-x86_64/bin/x86_64-w64-mingw32-clang++")
set(CMAKE_RC_COMPILER "/home/aspadien/CLionProjects/cubeRebuild/CrossCompiler/llvm-mingw-20251216-ucrt-ubuntu-22.04-x86_64/bin/x86_64-w64-mingw32-windres")

# Целевая архитектура
set(TRIPLE x86_64-w64-mingw32)
set(CMAKE_C_COMPILER_TARGET ${TRIPLE})
set(CMAKE_CXX_COMPILER_TARGET ${TRIPLE})

# Статическая лилинковка (чтобы exe работал везде)
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static -s")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-static -s")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-static -s")

# Блокируем поиск в системных папках Linux (чтобы не было ошибок cmath/cstdint)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)