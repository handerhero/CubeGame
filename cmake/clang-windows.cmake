# === АВТО-СКАЧИВАНИЕ КОМПИЛЯТОРА ===
set(COMPILER_VERSION "20240619") # Актуальная версия llvm-mingw (можно менять)
set(COMPILER_DIR "${CMAKE_CURRENT_LIST_DIR}/CrossCompiler/llvm-mingw-${COMPILER_VERSION}-ucrt-ubuntu-20.04-x86_64")
set(COMPILER_ARCHIVE "llvm-mingw-${COMPILER_VERSION}-ucrt-ubuntu-20.04-x86_64.tar.xz")
set(DOWNLOAD_URL "https://github.com/mstorsjo/llvm-mingw/releases/download/${COMPILER_VERSION}/${COMPILER_ARCHIVE}")

# Проверяем, есть ли компилятор
if(NOT EXISTS "${COMPILER_DIR}/bin/x86_64-w64-mingw32-clang")
    message(STATUS "Cross-compiler not found. Downloading llvm-mingw ${COMPILER_VERSION}...")

    # Создаем папку
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/CrossCompiler")

    # Скачиваем (может занять время!)
    file(DOWNLOAD "${DOWNLOAD_URL}" "${CMAKE_CURRENT_LIST_DIR}/CrossCompiler/${COMPILER_ARCHIVE}"
            SHOW_PROGRESS
            STATUS DOWNLOAD_STATUS
    )

    list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
    if(NOT STATUS_CODE EQUAL 0)
        message(FATAL_ERROR "Failed to download cross-compiler! Check internet connection.")
    endif()

    message(STATUS "Extracting cross-compiler...")
    execute_process(
            COMMAND tar -xf "${CMAKE_CURRENT_LIST_DIR}/CrossCompiler/${COMPILER_ARCHIVE}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/CrossCompiler"
            RESULT_VARIABLE EXIT_CODE
    )

    if(NOT EXIT_CODE EQUAL 0)
        message(FATAL_ERROR "Failed to extract compiler.")
    endif()

    message(STATUS "Cross-compiler ready!")
endif()

# === НАСТРОЙКА TOOLCHAIN ===
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Указываем пути к только что скачанному (или найденному) компилятору
set(TOOLCHAIN_BIN_DIR "${COMPILER_DIR}/bin")
set(CMAKE_C_COMPILER "${TOOLCHAIN_BIN_DIR}/x86_64-w64-mingw32-clang")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_BIN_DIR}/x86_64-w64-mingw32-clang++")
set(CMAKE_RC_COMPILER "${TOOLCHAIN_BIN_DIR}/x86_64-w64-mingw32-windres")

set(TRIPLE x86_64-w64-mingw32)
set(CMAKE_C_COMPILER_TARGET ${TRIPLE})
set(CMAKE_CXX_COMPILER_TARGET ${TRIPLE})

# Статическая лилинковка (важно для Windows EXE без DLL)
# llvm-mingw использует libc++ по умолчанию, так что -static работает отлично
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static -s -Wl,--allow-multiple-definition")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-static -s")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-static -s")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)