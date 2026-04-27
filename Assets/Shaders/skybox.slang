
// Referenced from https://github.com/SaschaWillems/Vulkan/blob/master/shaders/slang/texturecubemap/skybox.slang


struct CameraInfo {
    float4x4 view;
    float4x4 projection;
};

[[vk::binding(0, 0)]]
ConstantBuffer<CameraInfo> cameraData;

struct VSInput {
    float3 inPos;
    float3 inColor;
    float2 inTexCoord;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float3 UVW;
};

[Shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    // output.pos = float4(input.inPos, 1.0);
    float3 pos = input.inPos;
    pos.xy *= -1.0;
    output.UVW = pos;
    float4x4 viewMatrix = cameraData.view;
    // remove translation from view matrix
    viewMatrix[0][3] = 0.0;
    viewMatrix[1][3] = 0.0;
    viewMatrix[2][3] = 0.0;
    output.pos = mul(cameraData.projection, mul(viewMatrix, float4(pos, 1.0)));
    // float4x4 invProj = transpose(cameraData.projection);
    return output;
}

[[vk::binding(1, 0)]]
SamplerCube skyboxTexture;

[Shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_Target
{
    return skyboxTexture.Sample(vertIn.UVW);
}