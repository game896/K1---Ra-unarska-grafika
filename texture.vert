#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aTexCoord;

out vec2 TexCoords;

uniform float uX;
uniform float uY;
uniform float uScaleY;

void main()
{
    //vec2 pos = vec2(aPos.x, uY + (aPos.y - uY) * uScaleY);
    //gl_Position = vec4(pos + vec2(uX, 0.0), 0.0, 1.0);
    //TexCoords = aTexCoord;
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoords = aTexCoord;
}
