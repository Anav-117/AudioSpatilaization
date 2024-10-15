#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 pos;
layout(location = 2) in vec3 normal;

struct AmpVolume {
    
    float amp;

};

layout(std430, set = 1, binding = 0) readonly buffer AmplitudeIn {
   AmpVolume ampIn[ ];
};

int xExtent = 372;
int yExtent = 155;
int zExtent = 228;

float minX = 1920.95;
float minY = 1429.43;
float minZ = 1182.81;

layout(location = 0) out vec4 outColor;

vec3 lightPos = vec3(0.0, 5.0, 0.0);

void main() {
    
    vec3 lightDir = normalize(lightPos - pos);
    vec3 centerPos = normalize(pos - vec3(0.0));

    float diffuse = max(0.0, dot(lightDir, normal));

    float alpha = abs(dot(centerPos, normal) - 1.0);

    vec3 modifiedPos = (pos + vec3(minX, minY, minZ))/10.0;

    int yStride = xExtent;
    int zStride = (yExtent) * (xExtent);

    int index = int(modifiedPos.x + modifiedPos.y*yStride + modifiedPos.z*zStride);

    vec3 amp = vec3(ampIn[index].amp);

    outColor = vec4(diffuse * amp, alpha);
    //outColor = vec4(vec3(ampIn[index].amp), 1.0);
}