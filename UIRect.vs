#version 330 core
layout (location = 0) in vec2 aPos;

uniform vec2 uResolution;

void main()
{
    vec2 pos = aPos / uResolution * 2.0 - 1.0;
    pos.y *= -1.0; 
    gl_Position = vec4(pos, 0.0, 1.0);
}
