#version 430 core
out vec4 FragColor;

in vec3 outColor;
uniform float second;
void main()
{
    FragColor = vec4(outColor.x,second,outColor.z, 1.0);
}