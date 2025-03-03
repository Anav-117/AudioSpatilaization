#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 pos;
layout(location = 2) in vec3 normal;

layout(binding=0) uniform Transform {
    mat4 M;
    mat4 V;
    mat4 P;
    vec3 cameraPos;
    vec3 cameraFront;
} transform;

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

vec4 finalColor = vec4(0.0);

vec3 modelColor = vec3(0.5);

vec3 lightPos = vec3(0.0, 5.0, 0.0);

void main() {
    
    vec3 lightDir = normalize(lightPos - pos);
    vec3 centerPos = normalize(pos - vec3(0.0));

    float diffuse = max(0.5, max(0.0, dot(lightDir, normal)));

    float alpha = abs(dot(centerPos, normal) - 1.0);

    int yStride = xExtent;
    int zStride = (yExtent) * (xExtent);

    //outColor = vec4(diffuse * amp, alpha);
    //outColor = vec4(vec3(ampIn[index].amp), 1.0);

    vec3 localCamera = (inverse(transform.M) * vec4(transform.cameraPos, 1.0)).xyz;

    float dist = length(pos - localCamera);

    float sampleStep = dist/10.0;

    float accum = 0.0;

    int insideVol = 0;

    for (float i = 0; i < dist; i += sampleStep) {
        vec3 volumeSample = localCamera + i * normalize(pos - localCamera);
        
        ivec3 modifiedSample = ivec3((volumeSample + vec3(minX, minY, minZ))/10.0);

        int index = int(modifiedSample.x + modifiedSample.y*yStride + modifiedSample.z*zStride);

        accum += ampIn[index].amp / 10.0;

        if (accum >= 0.8) {
            accum = 0.8;
            break;
        }

    }

    //finalColor += vec4(1.0, 0.0, 0.0, accum);

    //if (i >= dist) {
    
    //}

    finalColor = mix(vec4(diffuse*modelColor, 1.0), vec4(1.0, 0.0, 0.0, 1.0), accum);

    vec3 modifiedSample = (pos + vec3(minX, minY, minZ))/10.0;

    int index = int(int(modifiedSample.x) + int(modifiedSample.y)*xExtent + int(modifiedSample.z)*xExtent * yExtent);

    float posAmp = (ampIn[index].amp);

    vec4 overlay = mix(vec4(1.0, 0.0, 0.0, 1.0), vec4(0.0, 1.0, 0.0, 1.0), posAmp);
    if (posAmp < 0.0) {
        overlay = vec4(0.0, 0.0, 1.0, 1.0);
    }

//    if (normalize(overlay) != normalize(vec4(0.0, 1.0, 0.0, 1.0) - vec4(1.0, 0.0, 0.0, 1.0))) {
//        overlay = vec4(1.0, 0.0, 1.0, 1.0);
//    }

    outColor = vec4(vec3(diffuse), 1.0) * overlay;

    //outColor = finalColor;

}