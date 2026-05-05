
// Referenced from https://github.com/SaschaWillems/Vulkan/blob/master/shaders/slang/texturecubemap/skybox.slang

struct CameraInfo {
    float4 position;
    float4x4 view;
    float4x4 projection;
    float4x4 jitteredProjection;
    float4x4 inverseView;
    float4x4 inverseProjection;
};

[[vk::binding(0, 0)]]
ConstantBuffer<CameraInfo> cameraData;

[[vk::binding(1, 0)]]
ConstantBuffer<CameraInfo> prevCameraData;

struct VSInput {
    float3 inPos;
    float3 inColor;
    float2 inTexCoord;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float3 viewDir;
    float2 velocity;
   // float3 prevViewDir;
};

float3 calcViewDir(float3 inputpos, float4x4 projection, float4x4 view) {
    //projection[1][1] *= -1; // Flip Y for Vulkan's coordinate system
    // Apply the diagonal inverse of the projection matrix equivalent

    float3 pos = inputpos;
    pos.xy *= float2(projection[0][0], projection[1][1]);
    pos.z = -1.0;
    float3 viewDir = mul(transpose(float3x3(view)), normalize(pos));

    // Multiply by the transposed (inverse) view matrix to reconstruct the world-space direction
    return viewDir;
}

float4 calcCameraVelocity(float4x4 prevProjection, float4x4 prevView, float4x4 inversePrevProjection, float4x4 inversePrevView, float3 viewDir) {
    // Reconstruct the world space position of the current pixel
    float3 worldPos = mul(inversePrevView, mul(inversePrevProjection, float4(viewDir, 0.0))).xyz;
    // Project the world space position into the previous frame's clip space
    float4 prevClipPos = mul(prevProjection, mul(prevView, float4(worldPos, 1.0)));
    // Perform perspective divide to get NDC coordinates
    prevClipPos.xyz /= prevClipPos.w;
    // The velocity is the difference between the current NDC position (which is just viewDir) and the previous NDC position
    return float4(prevClipPos.xyz - viewDir, 1.0);
}

float2 calcScreenSpaceVelocity(float3 viewDir) {
    // Need to calculate unjittered clip space position for current frame, and unjittered clip space position for previous frame, then take the difference.

    float4 currentPos = mul(cameraData.projection, mul(cameraData.view, float4(viewDir, 0.0)));
    float4 prevPos = mul(prevCameraData.projection, mul(prevCameraData.view, float4(viewDir, 0.0)));
    float2 currentNDC = currentPos.xy / currentPos.w;
    float2 currentClip = currentNDC * 0.5 + 0.5; // Convert from NDC to screen space

    float2 prevNDC = prevPos.xy / prevPos.w;
    float2 prevClip = prevNDC * 0.5 + 0.5; // Convert from NDC to screen space

    return currentClip - prevClip;
}


[Shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;

    // Set the NDC position directly (quad z is 0.0, forming a fullscreen quad)
    output.pos = float4(input.inPos, 1.0);

    float3 pos = input.inPos;
    float4x4 projection = cameraData.projection;
    float4x4 prevProjection = prevCameraData.inverseProjection;

    float3 viewDir = calcViewDir(pos, cameraData.inverseProjection, cameraData.view);
    output.viewDir = viewDir;
    output.velocity = calcScreenSpaceVelocity(viewDir);

    return output;
}

[[vk::binding(2, 0)]]
SamplerCube skyboxTexture;


struct FragOutput {
    float4 color : SV_Target0;
    float4 velocity : SV_Target1;
};

[Shader("fragment")]
FragOutput fragMain(VSOutput vertIn)
{
    FragOutput output;
    output.color = skyboxTexture.Sample(vertIn.viewDir);

    //float3 diff = vertIn.prevViewDir - vertIn.viewDir;
    output.velocity = float4(vertIn.velocity, 0.0, 1.0);

    return output;
}