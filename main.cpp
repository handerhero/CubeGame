// --- START OF FILE main.cpp ---

#define GLFW_INCLUDE_NONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <iostream>
#include <thread>
#include <vector>
#include <future>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/norm.hpp>

// Ваши заголовки
#include <GL/glext.h>

#include "Camera/Camera.hpp"
#include "ChunkSystem/ChunkGenerationManager.h"
#include "Debug/FPSCounter.hpp"
#include "Debug/GLDebug.hpp"
#include "IOReactions/KeyboardControl.h"
#include "IOReactions/MouseInteractions.h"
#include "Render/ChunksGpuManager.hpp"
#include "Render/GPUDrivenRenderTools.h"
#include "Render/ShaderCreate.h"
#include "ObjectsAndPhysic/AABB.h"
#include "ObjectsAndPhysic/Physic.hpp"
#include "Render/Window.hpp"

// Структура задачи загрузки (локальная для Main Thread)
struct UploadTask {
    std::shared_ptr<Chunk> chunk;
    std::vector<uint32_t> data;
};

class SimpleFramebuffer {
    GLuint fbo = 0;
    GLuint colorTex = 0;
    GLuint depthRbo = 0;
    int width = 0, height = 0;

public:
    ~SimpleFramebuffer() {
        Cleanup();
    }

    void Cleanup() {
        if (fbo) glDeleteFramebuffers(1, &fbo);
        if (colorTex) glDeleteTextures(1, &colorTex);
        if (depthRbo) glDeleteRenderbuffers(1, &depthRbo);
        fbo = colorTex = depthRbo = 0;
    }

    // Изменяет размер буфера, если нужно (ленивая инициализация)
    void Resize(const int w, const int h) {
        if (w == width && h == height && fbo != 0) return; // Размер не изменился
        if (w <= 0 || h <= 0) return;

        width = w;
        height = h;

        Cleanup();

        // 1. Создаем FBO
        glCreateFramebuffers(1, &fbo);

        // 2. Создаем текстуру цвета (куда будем рисовать)
        glCreateTextures(GL_TEXTURE_2D, 1, &colorTex);
        glTextureStorage2D(colorTex, 1, GL_RGBA8, width, height);
        // Важно: GL_NEAREST даст пиксельный вид (Minecraft style).
        // GL_LINEAR даст мыльный вид (Upscaling).
        glTextureParameteri(colorTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(colorTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // Прикрепляем цвет к FBO
        glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, colorTex, 0);

        // 3. Создаем буфер глубины (Renderbuffer), он нужен для Z-теста
        glCreateRenderbuffers(1, &depthRbo);
        glNamedRenderbufferStorage(depthRbo, GL_DEPTH_COMPONENT24, width, height);
        glNamedFramebufferRenderbuffer(fbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRbo);

        // Проверка
        if (glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "FBO Error!" << std::endl;
        }
    }

    void Bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, width, height); // Важно: вьюпорт под размер FBO
    }

    // Копирует содержимое FBO на экран с растягиванием
    void BlitToScreen(int screenWidth, int screenHeight) const {
        // Возвращаемся к дефолтному буферу (экран)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, screenWidth, screenHeight);

        // Магия копирования: берем из fbo (read) и кидаем в 0 (draw)
        glBlitNamedFramebuffer(fbo, 0,
            0, 0, width, height,          // Откуда (весь FBO)
            0, 0, screenWidth, screenHeight, // Куда (весь экран)
            GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
};

class VoxelGame {
public:
    std::atomic<bool> programIsRunning{true};
    Window* window;
    KeyboardControl keyboard_control;
    glm::vec3 physicsVector = glm::vec3(0.0f);
    AABB *playerAABB;
    bool onGround = false;
    double timeCollector = 0;

    VoxelGame() : threadPool(std::thread::hardware_concurrency() / 2) {
        window = new Window(camera,"My Game");
        programIsRunning = true;
        physic_ = new Physic(camera);
        playerAABB = new AABB(camera.pos, 1.8f,0.8f,0.8f);
    }

    VoxelGame(const VoxelGame&) = delete;
    VoxelGame& operator=(const VoxelGame&) = delete;

    VoxelGame(VoxelGame&&) = delete;
    VoxelGame& operator=(VoxelGame&&) = delete;


    ~VoxelGame() {
        Cleanup();
    }

    void Init() {
        #ifndef NDEBUG
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(MessageCallback, 0);
        #endif

        glfwSwapInterval(0);
        glfwSetInputMode(window->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorPosCallback(window->window, mouse_callback);

        LoadShaders();
        LoadTextures();

        gpuManager = std::make_unique<ChunkGpuManager>(renderDistanceXZ+3,(renderHeightY+2)*2);

        // Запуск потоков
        int workers = std::max(1u, std::thread::hardware_concurrency() - 2); // Оставим пару ядер системе
        for(int i = 0; i < workers; ++i) {
            workerThreads.emplace_back(chunkWorker);
        }

        finderThread = std::thread(chunkFinder, std::ref(loadedChunks));

        camera.pos = glm::vec3(16, 40, -40);

        // Предварительная настройка GL состояний
        glBindVertexArray(gpuManager->vao);
        // Заметьте: visibleSSBO здесь больше не нужен
        glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
    }

    void Run() {
        FPSCounterAdvanced fpsCounter;

        while (!glfwWindowShouldClose(window->window)) {
            // 1. Обновление позиции для генератора
            {
                std::lock_guard lock(playerPosMutex);
                currentPlayerChunk = glm::floor(camera.pos / 32.0);
            }

            // 2. Логика чанков
            ProcessNewChunks();
            ProcessUnloadQueue();
            ScheduleMeshing();
            UploadToGPU();

            // 3. Рендер
            RenderFrame();

            fpsCounter.update(window->window);
            glfwSwapBuffers(window->window);
            glfwPollEvents();
        }
    }

private:
    std::vector<std::shared_ptr<Chunk>> chunksToMeshQueue;
    Camera camera;
    ChunkMap loadedChunks;
    Physic *physic_;
    PriorityThreadPool threadPool;
    std::unique_ptr<ChunkGpuManager> gpuManager;

    GLuint renderProgram = 0;
    GLuint computeProgram = 0;
    GLuint texture = 0;

    std::vector<std::thread> workerThreads;
    std::thread finderThread;

    SimpleFramebuffer renderFbo;
    float renderScale = 1.0f;

    std::vector<UploadTask> uploadQueue;
    std::mutex uploadMutex;

    // renderList больше не нужен для отрисовки, но оставим для совместимости логики
    std::vector<Chunk*> renderList;
    std::vector<std::shared_ptr<Chunk>> changedChunks;

    void AddToRenderList(Chunk* chunk) {
        if (chunk->renderListIndex != -1) return;
        chunk->renderListIndex = renderList.size();
        renderList.push_back(chunk);
    }

    void RemoveFromRenderList(Chunk* chunk) {
        if (chunk->renderListIndex == -1) return;
        size_t idx = chunk->renderListIndex;
        size_t lastIdx = renderList.size() - 1;
        if (idx != lastIdx) {
            Chunk* lastChunk = renderList[lastIdx];
            renderList[idx] = lastChunk;
            lastChunk->renderListIndex = idx;
        }
        renderList.pop_back();
        chunk->renderListIndex = -1;
    }

    void LoadShaders() {
        // Убедитесь, что файлы называются правильно
        renderProgram = ShaderCreate("shaders/shader.vert", "shaders/shader.frag");
        // Здесь должен быть shaders/cull_mdi.comp из моего предыдущего сообщения
        computeProgram = ShaderCreate("shaders/shader.comp");

        glUseProgram(renderProgram);
        glUniform1i(glGetUniformLocation(renderProgram, "tex0"), 0);
        glUniform3f(glGetUniformLocation(renderProgram, "lightPos"), -20.0f, 0.0f, -12.0f);
        glUniform3f(glGetUniformLocation(renderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
    }

    void LoadTextures() {
        int* texArr[16];
        texArr[1] = static_cast<int *>(loadTextureRGBA("textures/Dirt.png"));
        texArr[3] = static_cast<int *>(loadTextureRGBA("textures/Stone.png"));
        texArr[5] = static_cast<int *>(loadTextureRGBA("textures/Grass.png"));

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

        int width = 16, height = 16, sides = 6, elements = 3;
        int layers = 6 * sides;

        glTexStorage3D(GL_TEXTURE_2D_ARRAY, 5, GL_RGBA8, width, height, layers);

        for (int i = 0; i < elements; ++i) {
            for (int j = 0; j < sides; ++j) {
                glTexSubImage3D(
                    GL_TEXTURE_2D_ARRAY, 0, 0, 0, j + (i*6*2+6), width, height, 1,
                    GL_RGBA, GL_UNSIGNED_BYTE, texArr[i*2+1] + (j * 256)
                );
            }
            stbi_image_free(texArr[i*2+1]);
        }

        float maxAniso = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::min(maxAniso, 8.0f));
        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    }

    void ProcessNewChunks() {
        std::vector<std::shared_ptr<Chunk>> batch = changedChunks;
        changedChunks.clear();
        {
            std::lock_guard lock(voxelDataMutex);
            int count = 0;
            while (!voxelDataQueue.empty() && count++ < 256) {
                batch.push_back(voxelDataQueue.front());
                voxelDataQueue.pop();
            }
        }

        for (auto& newChunk : batch) {
            auto oldChunk = loadedChunks.tryGet(newChunk->worldPosition);

            if (oldChunk && oldChunk == newChunk) {
                // Обновление существующего
            }
            else {
                if (oldChunk) {
                    RemoveFromRenderList(oldChunk.get());
                    gpuManager->freeChunk(oldChunk.get());
                    loadedChunks.erase(newChunk->worldPosition);
                }
                loadedChunks.insert(newChunk->worldPosition, newChunk);
            }

            if (!newChunk->needsMeshUpdate) {
                newChunk->needsMeshUpdate = true;
                chunksToMeshQueue.push_back(newChunk);
            }

            const glm::ivec3 offs[] = {{-1,0,0},{1,0,0},{0,-1,0},{0,1,0},{0,0,-1},{0,0,1}};
            for(auto& o : offs) {
                if(auto n = loadedChunks.tryGet(newChunk->worldPosition + o)) {
                    if (!n->needsMeshUpdate) {
                        n->needsMeshUpdate = true;
                        chunksToMeshQueue.push_back(n);
                    }
                }
            }
            std::lock_guard glock(generationMutex);
            pendingGeneration.erase(newChunk->worldPosition);
        }
    }

    void ProcessUnloadQueue() {
        std::vector<glm::ivec3> batch;
        {
            std::lock_guard lock(unloadMutex);
            while(!unloadQueue.empty()) {
                batch.push_back(unloadQueue.front());
                unloadQueue.pop();
            }
        }

        for(auto& pos : batch) {
            auto ptr = loadedChunks.tryGet(pos);
            if(ptr) {
                RemoveFromRenderList(ptr.get());
                gpuManager->freeChunk(ptr.get());
                loadedChunks.erase(pos);
            }
        }
    }

    void ScheduleMeshing() {
        if (chunksToMeshQueue.empty()) return;

        glm::vec3 pPos = camera.pos;
        std::sort(chunksToMeshQueue.begin(), chunksToMeshQueue.end(),
            [pPos](const std::shared_ptr<Chunk>& a, const std::shared_ptr<Chunk>& b) {
                float distA = glm::distance2(glm::vec3(a->worldPosition * 32 + 16), pPos);
                float distB = glm::distance2(glm::vec3(b->worldPosition * 32 + 16), pPos);
                return distA < distB;
            }
        );

        for(auto& chunk : chunksToMeshQueue) {
            if(chunk->needsMeshUpdate) {
                chunk->needsMeshUpdate = false;
                glm::vec3 cPos = glm::vec3(chunk->worldPosition * 32 + 16);
                int priority = (int)glm::distance2(cPos, pPos);
                const std::shared_ptr<Chunk>& sharedPtr = chunk;

                threadPool.enqueue(priority, [this, sharedPtr]() {
                    if(!programIsRunning) return;
                    if (!loadedChunks.contains(sharedPtr->worldPosition)) return;

                    auto mesh = BuildChunkMesh(sharedPtr.get(), loadedChunks);

                    std::lock_guard lock(uploadMutex);
                    uploadQueue.push_back({sharedPtr, std::move(mesh)});
                });
            }
        }
        chunksToMeshQueue.clear();
    }

    void UploadToGPU() {
        std::lock_guard lock(uploadMutex);
        int uploaded = 0;
        for(auto it = uploadQueue.begin(); it != uploadQueue.end(); ) {
            auto existing = loadedChunks.tryGet(it->chunk->worldPosition);
            if(existing && existing == it->chunk) {
                gpuManager->uploadChunk(it->chunk.get(), it->data);
                AddToRenderList(it->chunk.get());
            }
            it = uploadQueue.erase(it);
            if(++uploaded > 256) break;
        }
    }

    void RenderFrame() {
        gpuManager->recycleZombies();

        auto now = std::chrono::system_clock::now();
        if (timeCollector == 0) {
            timeCollector = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        }
        const double dt = (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() - timeCollector) / 1000;
        timeCollector = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        physic_->makeTick(keyboard_control,changedChunks,camera,dt,loadedChunks,*window);

        // 1. Подготовка FBO
        int winWidth, winHeight;
        glfwGetFramebufferSize(window->window, &winWidth, &winHeight);
        if (winWidth == 0 || winHeight == 0) return;

        const int renderW = static_cast<int>(static_cast<float>(winWidth) * renderScale);
        const int renderH = static_cast<int>(static_cast<float>(winHeight) * renderScale);
        renderFbo.Resize(renderW, renderH);

        renderFbo.Bind();
        glClearColor(0.5f, 0.8f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 2. Матрицы
        float aspectRatio = static_cast<float>(renderW) / static_cast<float>(renderH);
        float nearPlane = 4096.0f;
        glm::dmat4 projection = glm::perspective(glm::radians(84.0f), aspectRatio, nearPlane, 0.125f);
        glm::dmat4 view = glm::lookAt(glm::dvec3(0), glm::dvec3(camera.camera->forward()), glm::dvec3(camera.camera->up()));
        glm::mat4 viewProj = glm::mat4(projection * view);

        glm::ivec3 playerChunkPos = glm::floor(camera.pos / 32.0f);


        // ==========================================
        // ЭТАП 1: ГЕНЕРАЦИЯ КОМАНД (Compute Shader)
        // ==========================================

        // 1. Сброс Атомарного Счетчика (НОВЫЙ МЕТОД - GPU COMMAND)
        // Мы говорим GPU: "Заполни этот буфер нулями". CPU не касается памяти.
        // GL_R32UI означает, что мы работаем с unsigned int (32 бита).
        // nullptr в конце означает "заполни нулями".
        glClearNamedBufferData(gpuManager->parameterBuffer, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);

        // 2. Compute Shader
        glUseProgram(computeProgram);

        glUniformMatrix4fv(glGetUniformLocation(computeProgram, "viewProj"), 1, GL_FALSE, glm::value_ptr(viewProj));
        glUniform3f(glGetUniformLocation(computeProgram, "camPos"), camera.pos.x, camera.pos.y, camera.pos.z);
        glUniform1ui(glGetUniformLocation(computeProgram, "totalChunks"), gpuManager->maxChunksCapacity);

        // Bindings:
        // 0: Chunk Metadata (Read)
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gpuManager->chunkInfoBuffer);
        // 3: Command Buffer (Write - массив команд)
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, gpuManager->indirectContext.commandBuffer);
        // 4: Parameter Buffer (Atomic Counter)
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, gpuManager->parameterBuffer);

        // Запуск: 1 поток на 1 чанк (группы по 64)
        int numGroups = (gpuManager->maxChunksCapacity + 63) / 64;
        glDispatchCompute(numGroups, 1, 1);

        // ==========================================
        // ЭТАП 2: БАРЬЕР
        // ==========================================
        // Ждем записи команд и счетчика
        glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

        // ==========================================
        // ЭТАП 3: РЕНДЕР (MDI Count)
        // ==========================================

        glUseProgram(renderProgram);

        glUniformMatrix4fv(glGetUniformLocation(renderProgram, "viewProjection"), 1, GL_FALSE, glm::value_ptr(viewProj));
        glUniform3f(glGetUniformLocation(renderProgram, "cameraPos"), camera.pos.x, camera.pos.y, camera.pos.z);
        glUniform3i(glGetUniformLocation(renderProgram, "playerChunkPos"), playerChunkPos.x, playerChunkPos.y, playerChunkPos.z);

        // Bindings:
        // 0: Metadata (Read) - нужно для gl_BaseInstance
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gpuManager->chunkInfoBuffer);
        // 2: Geometry (Read - uvec2/uint)
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, gpuManager->vertexSSBO);

        glBindVertexArray(gpuManager->vao);

        // Биндим буфер команд как INDIRECT буфер
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, gpuManager->indirectContext.commandBuffer);
        // Биндим буфер счетчика как PARAMETER буфер
        glBindBuffer(GL_PARAMETER_BUFFER_ARB, gpuManager->parameterBuffer);

        // РИСУЕМ!
        // GPU читает count из parameterBuffer и исполняет команды из commandBuffer
        glMultiDrawArraysIndirectCount(GL_TRIANGLE_STRIP,
                                       0, // offset in commands
                                       0, // offset in count buffer
                                       gpuManager->maxChunksCapacity, // max draw count
                                       0  // stride
        );

        renderFbo.BlitToScreen(winWidth, winHeight);
    }

    void Cleanup() {
        // 1. Сначала останавливаем логику
        programIsRunning = false;
        running = false;

        // 2. Будим ВСЕ потоки, чтобы они вышли из wait()
        // Важно: если у вас есть generationCV и meshingCV, будите оба
        generationCV.notify_all();

        // Если в ChunkGenerationManager есть свои очереди с mutex/cv,
        // убедитесь, что они тоже получают сигнал остановки.

        // 3. Ждем завершения потоков
        for(auto& t : workerThreads) {
            if(t.joinable()) t.join();
        }
        if(finderThread.joinable()) {
            finderThread.join();
        }

        // 4. !!! ВАЖНО !!! Уничтожаем GPU ресурсы ПОКА ЕСТЬ КОНТЕКСТ (Window)
        // Если уничтожить window первым, деструктор gpuManager упадет или зависнет драйвер.
        gpuManager.reset(); // Явно вызываем деструктор менеджера

        renderFbo.Cleanup(); // И FBO тоже
        glDeleteTextures(1, &texture);
        glDeleteProgram(renderProgram);
        glDeleteProgram(computeProgram);

        // 5. Удаляем физику и прочее
        delete physic_;
        delete playerAABB;

        // 6. Только в самом конце удаляем окно (Context)
        delete window;
    }
};

int main() {
    VoxelGame game;
    game.Init();
    game.Run();
    return 0;
}