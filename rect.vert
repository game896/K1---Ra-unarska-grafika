#version 330 core

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec4 inCol;
out vec4 chCol;

uniform float uX;
uniform float uY;
uniform float uScaleY;  

void main()
{
    vec2 pos = inPos;

    pos.y *= uScaleY;

    gl_Position = vec4(pos.x + uX, pos.y + uY, 0.0, 1.0);
    chCol = inCol;
}
