
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
    float3 viewDir;
};

[Shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;

    // Set the NDC position directly (quad z is 0.0, forming a fullscreen quad)
    output.pos = float4(input.inPos, 1.0);

    float3 pos = input.inPos;

    // Apply the diagonal inverse of the projection matrix equivalent
    float invProjX = 1.0 / cameraData.projection[0][0];
    float invProjY = 1.0 / cameraData.projection[1][1];

    pos.x *= invProjX;
    pos.y *= invProjY;
    pos.z = -1.0;

    // Extract the 3x3 rotation view matrix
    float3x3 view3x3 = float3x3(
        cameraData.view[0].xyz,
        cameraData.view[1].xyz,
        cameraData.view[2].xyz
    );

    // Multiply by the transposed (inverse) view matrix to reconstruct the world-space direction
    output.viewDir = mul(transpose(view3x3), normalize(pos));

    return output;
}

[[vk::binding(1, 0)]]
SamplerCube skyboxTexture;

[Shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_Target
{
    return skyboxTexture.Sample(vertIn.viewDir);
}