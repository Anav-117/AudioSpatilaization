#version 450

layout(location = 0) in vec3 pos_attrib;
layout(location = 1) in vec3 normal_attrib;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 pos;
layout(location = 2) out vec3 normal;

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

layout(binding=0) uniform Transform {
    mat4 M;
    mat4 V;
    mat4 P;
    vec3 cameraPos;
    vec3 cameraFront;
} transform;

void main() {
    gl_Position = transform.P * transform.V * transform.M * vec4(pos_attrib, 1.0);
    
    pos = pos_attrib; //(transform.M * vec4(pos_attrib, 1.0)).xyz;
    normal = normalize((transform.M * vec4(normal_attrib, 1.0)).xyz);

    //fragColor = colors[int(gl_VertexIndex) % 3];
}