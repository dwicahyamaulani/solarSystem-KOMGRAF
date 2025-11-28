#version 330 core

uniform float uTime;

out vec4 FragColor;

void main()
{
    // kilap random berdasarkan posisi pixel + waktu
    float flicker = abs(sin(uTime * 10.0 + gl_FragCoord.x * 0.05));

    // warna putih kebiruan
    vec3 color = mix(vec3(0.4, 0.6, 1.0), vec3(1.0), flicker);

    FragColor = vec4(color, 1.0);
}
