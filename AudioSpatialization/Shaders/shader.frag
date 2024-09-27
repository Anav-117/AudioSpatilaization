#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 pos;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec4 outColor;

vec3 lightPos = vec3(0.0, 10.0, 0.0);

void main() {
    
    vec3 lightDir = normalize(lightPos - pos);

    float diffuse = max(0.0, dot(lightDir, normal));

    outColor = diffuse * vec4(0.5);
}