
// Referenced from https://github.com/SaschaWillems/Vulkan/blob/master/shaders/slang/texturecubemap/skybox.slang

struct CameraInfo {
    float4 position;
    float4x4 view;
    float4x4 projection;
    float4x4 jitteredProjection;
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
    float3 prevViewDir;
};

float3 calcViewDir(float3 pos, float4x4 projection, float4x4 view) {
    projection[1][1] *= -1; // Flip Y for Vulkan's coordinate system
    // Apply the diagonal inverse of the projection matrix equivalent
    float invProjX = 1.0 / projection[0][0];
    float invProjY = 1.0 / projection[1][1];
    pos.x *= invProjX;
    pos.y *= invProjY;
    pos.z = -1.0;
    // Extract the 3x3 rotation view matrix
    float3x3 view3x3 = float3x3(
        view[0].xyz,
        view[1].xyz,
        view[2].xyz
    );
    // Multiply by the transposed (inverse) view matrix to reconstruct the world-space direction
    return mul(transpose(view3x3), normalize(pos));
}



[Shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;

    // Set the NDC position directly (quad z is 0.0, forming a fullscreen quad)
    output.pos = float4(input.inPos, 1.0);

    float3 pos = input.inPos;
    float4x4 projection = cameraData.projection;
    float4x4 prevProjection = prevCameraData.projection;

    //projection[1][1] *= -1; // Flip Y for Vulkan's coordinate system
    output.viewDir = calcViewDir(pos, projection, cameraData.view);
    output.prevViewDir = calcViewDir(pos, prevProjection, prevCameraData.view);
    // Apply the diagonal inverse of the projection matrix equivalent
    //float invProjX = 1.0 / projection[0][0];
    //float invProjY = 1.0 / projection[1][1];



    //pos.x *= invProjX;
    //pos.y *= invProjY;
    //pos.z = -1.0;

    // Extract the 3x3 rotation view matrix
    //float3x3 view3x3 = float3x3(
        //cameraData.view[0].xyz,
        //cameraData.view[1].xyz,
      //  cameraData.view[2].xyz
    //);

    // Multiply by the transposed (inverse) view matrix to reconstruct the world-space direction
    //output.viewDir = mul(transpose(view3x3), normalize(pos));

    return output;
}

[[vk::binding(2, 0)]]
SamplerCube skyboxTexture;


struct FragOutput {
    float4 color : SV_Target;
    float4 velocity : SV_Target1;
};

[Shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_Target
{
    FragOutput output;
    output.color = skyboxTexture.Sample(vertIn.viewDir);
    float3 diff = vertIn.prevViewDir - vertIn.viewDir;
    output.velocity = float4(diff, 0.0f);

    return output.color;
}