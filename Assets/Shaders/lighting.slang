struct VSInput {
    float3 inPos;
    float3 inColor;
    float2 inTexCoord;
};

struct lightTransmition {
    float4 position;
    float4 direction;
    float4 colour;
};
[[vk::binding(0, 0)]]
ConstantBuffer<lightTransmition> lightTransmitions;

struct CameraInfo {
    float4 position;
    float4x4 view;
    float4x4 projection;
};

struct PushConstants {
    uint lightCount;
};

[[vk::binding(1, 0)]]
ConstantBuffer<CameraInfo> cameraData;

[[vk::push_constant]]
ConstantBuffer<PushConstants> lightNum;


struct VSOutput
{
    float4 pos : SV_Position;
    float3 color;
    float2 texCoord;
}

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    output.pos = mul(cameraData.projection, mul(cameraData.view, float4(input.inPos, 1.0)));
    output.color = input.inColor;
    output.texCoord = input.inTexCoord;
    return output;
}

[[vk::binding(2, 0)]]
Sampler2D shadowMap;
[[vk::binding(3, 0)]]
Sampler2D gColour;
[[vk::binding(4, 0)]]
Sampler2D gNormal;
[[vk::binding(5, 0)]]
Sampler2D gMaterial;
[[vk::binding(6, 0)]]
Sampler2D gPosition;




[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_Target
{
    float4 colour = gColour.Sample(vertIn.texCoord);
    float3 normal = normalize(gNormal.Sample(vertIn.texCoord).xyz);
    float4 material = gMaterial.Sample(vertIn.texCoord);
    float4 fragPos = gPosition.Sample(vertIn.texCoord);

    float roughness = material.r;
    float metallic = material.g;
    float3 albedo = colour.rgb;

    // Discard or return black if hitting the background/sky (no geometry was rendered here)
    if (colour.a < 0.01) {
        return float4(0.0, 0.0, 0.0, 1.0);
    }
    float3 lighting = albedo * 0.1; // Ambient term
    return float4(lighting, 1.0);

}