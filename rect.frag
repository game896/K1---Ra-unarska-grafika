#version 330 core
in vec4 chCol;
out vec4 outCol;

uniform vec4 overrideColor; // -1 znaci bez overridea

void main()
{
    if (overrideColor.r >= 0.0) // ako je r >= 0, koristi override
        outCol = overrideColor;
    else
        outCol = chCol;
}
