#version 330 core
out vec4 FragColor;

uniform vec3 lightColor;

void main()
{
    // emissive glow — slightly brighter than 1 for bloom feel
    FragColor = vec4(lightColor * 1.2, 1.0);
}
