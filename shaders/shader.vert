#version 460 core

struct ChunkMetadata {
    int X, Y, Z;
    uint instanceCount;
    uint first;
    uint number;
    int pad1, pad2;
};

// Читаем метаданные (позицию чанка берем отсюда)
layout(std430, binding = 0) readonly restrict buffer Chunks { ChunkMetadata chunks[]; };
// Читаем геометрию (uvec2)
layout(std430, binding = 2) readonly restrict buffer AllData { uvec2 allData[]; };

uniform mat4 viewProjection;
uniform vec3 cameraPos;
uniform ivec3 playerChunkPos;

out vec3 uv;

// !!! ИСПРАВЛЕНИЕ 1: Добавлен массив координат углов квада
const vec2 quadCornerUV[4] = vec2[](vec2(0,0), vec2(1,0), vec2(0,1), vec2(1,1));

void main() {
    // 1. Получаем метаданные чанка по gl_BaseInstance (который заполнил Compute Shader)
    ChunkMetadata chunk = chunks[gl_BaseInstance];

    // 2. Получаем данные вершины
    uint dataIndex = (chunk.first / 2) + gl_InstanceID;
    uvec2 data = allData[dataIndex];
    uint low = data.x;
    uint high = data.y;

    // --- 3. РАСПАКОВКА ПАРАМЕТРОВ ВОКСЕЛЯ ---
    float x = float((low >> 0) & 31u);
    float y = float((low >> 5) & 31u);
    float z = float((low >> 10) & 31u);
    uint face = (low >> 15) & 7u;
    float w = float(((low >> 18) & 31u) + 1u);
    float h = float(((low >> 23) & 31u) + 1u);

    // Распаковка BlockID (разбит между low и high)
    uint blockId = (low >> 28u) | ((high & 15u) << 4u);

    // --- 4. РАСЧЕТ ПОЗИЦИИ ЧАНКА (ИСПРАВЛЕНИЕ 2) ---
    // Мы не распаковываем dx/dy/dz, так как вершины статичны.
    // Мы вычисляем смещение чанка относительно игрока прямо здесь.
    vec3 chunkDelta = vec3(chunk.X, chunk.Y, chunk.Z) - vec3(playerChunkPos);

    // --- 5. ГЕНЕРАЦИЯ ГЕОМЕТРИИ ---
    float u = quadCornerUV[gl_VertexID % 4].x;
    float v = quadCornerUV[gl_VertexID % 4].y;

    vec3 pos = vec3(x, y, z);

    switch (face) {
        case 0: // +Z (Front)
        pos += vec3(w * u, h * v, 1.0);
        break;
        case 1: // -Z (Back)
        pos += vec3(w * (1.0 - u), h * v, 0.0);
        break;
        case 3: // +Y (Top)
        pos += vec3(w * u, 1.0, h * v) - vec3(0,1,0); // Коррекция под вашу логику
        break;
        case 2: // -Y (Bottom)
        pos += vec3(w * u, 0.0, h * (1.0 - v)) + vec3(0,1,0); // Коррекция под вашу логику
        break;
        case 4: // +X (Right)
        pos += vec3(1.0, h * v, w * (1.0 - u));
        break;
        case 5: // -X (Left)
        pos += vec3(0.0, h * v, w * u);
        break;
    }

    // --- 6. МИРОВЫЕ КООРДИНАТЫ ---
    // Позиция вершины относительно угла чанка игрока
    vec3 vertexRel = (chunkDelta * 32.0) + pos;

    // Камера относительно того же угла
    // (playerChunkPos уже учтен в chunkDelta, поэтому здесь просто вычитаем остаток позиции камеры)
    vec3 camRel = cameraPos - (vec3(playerChunkPos) * 32.0);

    gl_Position = viewProjection * vec4(vertexRel - camRel, 1.0);

    // --- 7. ТЕКСТУРЫ ---
    vec2 texCoord = vec2(u * w, v * h);

    // Исправление ориентации текстур для стен
    if (face != 2 && face != 3) {
        texCoord.y = h - texCoord.y;
    }

    uv = vec3(texCoord, float(blockId * 6u + face));
}