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

    float diffuse = max(0.0, dot(lightDir, normal));

    float alpha = abs(dot(centerPos, normal) - 1.0);

    int yStride = xExtent;
    int zStride = (yExtent) * (xExtent);

    //outColor = vec4(diffuse * amp, alpha);
    //outColor = vec4(vec3(ampIn[index].amp), 1.0);

    vec3 localCamera = (inverse(transform.M) * vec4(transform.cameraPos, 1.0)).xyz;

    float dist = length(pos - localCamera);

    float step = dist/10.0;

    float accum = 0.0;
    float volumeAlpha = 0.0;

    float i;
    int insideVol = 0;

    for (i = 0; i < dist; i += step) {
        vec3 volumeSample = localCamera + i * normalize((transform.M * vec4(pos, 1.0)).xyz - transform.cameraPos);
        
        vec3 modifiedSample = (volumeSample + vec3(minX, minY, minZ))/10.0;

        int index = int(modifiedSample.x + modifiedSample.y*yStride + modifiedSample.z*zStride);

        accum += ampIn[index].amp;

        if (ampIn[index].amp > 0) {
            insideVol = 1;
        }
        if (insideVol == 1 && ampIn[index].amp == 0) {
            break;
        }

        if (accum >= 0.8) {
            accum = 0.8;
            break;
        }

        //ampIn[index] = -1.0;

    }

    //finalColor += vec4(1.0, 0.0, 0.0, accum);

    //if (i >= dist) {
    
    //}

    finalColor = mix(vec4(diffuse*modelColor, 1.0), vec4(1.0, 0.0, 0.0, 1.0), accum);

    outColor = finalColor;

    vec3 modifiedSample = (pos + vec3(minX, minY, minZ))/10.0 - vec3(1.0);

    int index = int(int(modifiedSample.x) + int(modifiedSample.y)*yStride + int(modifiedSample.z)*zStride);

    float posAmp = (ampIn[index].amp);


    if (posAmp == 1) {
        outColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
    else {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
    }

    outColor = mix(vec4(0.0, 0.0, 0.0, 1.0), vec4(1.0, 0.0, 0.0, 1.0), posAmp);
    
    //outColor = vec4((transform.M * vec4(pos, 1.0)).xyz + vec3(minX, minY, minZ), 1.0);

    //outColor = vec4(vec3(accum), 1.0);

}