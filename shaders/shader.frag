#version 460 core

// Вход: цвет, интерполированный по поверхности грани
in vec3 uv;

uniform sampler2DArray tex0;
// Выход: финальный цвет пикселя
out vec4 FragColor;

void main()
{
    FragColor = texture(tex0, uv);
}

