
struct GPUObjectData {
    float4x4 model;
};

struct CameraInfo {
    float4x4 view;
    float4x4 projection;
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
    float3 viewPos;
    float3 color;
    float2 texCoord;
};

[Shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;

    // 1. Get the correct index using the push constant base + the current vulkan instance
    uint objectIndex = pc.instanceBaseOffset + input.instanceID;

    // 2. Fetch the specific model matrix for this object
    float4x4 modelMatrix = objectBuffer[objectIndex].model;

    // 3. Apply Model, View, and Projection matrices to the vertex position
    // Note: Slang/HLSL uses mul(matrix, vector) or mul(vector, matrix) depending on matrix majority.
    // Standard GLM -> Vulkan usually expects mapping vector * matrix or standard mul(MVP, pos).
    // float4 worldPos = mul(modelMatrix, float4(input.inPos, 1.0));
    // float4 viewPos = mul(cameraData.view, worldPos);
    // output.pos = mul(cameraData.projection, viewPos);
    float4x4 projection = cameraData.projection;
    projection[1][1] *= -1; // Flip Y for Vulkan's coordinate system
    // Pass through vertex colors and UVs
    float4 pos = mul(projection, mul(cameraData.view, mul(modelMatrix, float4(input.inPos, 1.0))));
    output.pos = pos;
    output.viewPos = pos.xyz;
    output.color = input.inColor;
    output.texCoord = input.inTexCoord;

    return output;
}

struct MaterialData {
    float roughness;
    float metallic;
};

[[vk::binding(2, 0)]]
ConstantBuffer<MaterialData> materialData;

[[vk::binding(3, 0)]]
Sampler2D colourTexure;

[[vk::binding(4, 0)]]
Sampler2D normalTexture;

struct FragOutput {
    float4 colour : SV_Target0;
    float4 normal : SV_Target1;
    float4 material : SV_Target2;
    float4 pos : SV_Target3;
};


[Shader("fragment")]
FragOutput fragMain(VSOutput vertIn)
{    float4 col = colourTexure.Sample(vertIn.texCoord);
    float4 norm = normalTexture.Sample(vertIn.texCoord);
    float4 mat = float4(materialData.roughness, materialData.metallic, 0.0, 1.0);
    // Optionally multiply by vertIn.color if your GLTF models have vertex colors!
    FragOutput output;
    output.colour = col;
    output.normal = norm;
    output.material = mat;
    output.pos = float4(vertIn.viewPos, 1.0);

    return output;
}