struct GPUObjectData {
    float4x4 model;
};

struct CameraInfo {
    float4 position;
    float4x4 view;
    float4x4 projection;
    float4x4 jitteredProjection;
};

struct PushConstants {
    uint instanceBaseOffset;
};


[[vk::binding(0, 0)]]
StructuredBuffer<GPUObjectData> objectBuffer;

[[vk::binding(1, 0)]]
ConstantBuffer<CameraInfo> cameraData;

[[vk::push_constant]]
ConstantBuffer<PushConstants> pc;

struct VSInput {
    // Slang will automatically assign Vulkan locations 0, 1, and 2 based on order
    float3 inPos;
    float3 inColor;
    float2 inTexCoord;
    uint instanceID : SV_InstanceID;
};



struct VSOutput
{
    float4 pos : SV_Position;
};

[Shader("vertex")]
VSOutput vertMain(VSInput input) {

    uint objectIndex = pc.instanceBaseOffset + input.instanceID;
    float4x4 modelMatrix = objectBuffer[objectIndex].model;
    float4x4 projection = cameraData.jitteredProjection;
    projection[1][1] *= -1;

    VSOutput output;
    output.pos = mul(projection, mul(cameraData.view, mul(modelMatrix, float4(input.inPos, 1.0))));
    return output;
};

[Shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target {
    // For shadow mapping, we typically only care about depth, so we can output a simple color or even just white.
    return float4(1.0, 1.0, 1.0, 1.0); // Output white for all fragments
};
