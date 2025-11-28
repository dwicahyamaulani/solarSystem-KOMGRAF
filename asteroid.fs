#version 330 core
out vec4 FragColor;

uniform float uTime;

void main()
{
    // kelap-kelip halus 0.3 s.d 1.0
    float flicker = 0.3 + abs(sin(uTime * 5.0));

    FragColor = vec4(1.0, 1.0, 1.0, flicker);
}
