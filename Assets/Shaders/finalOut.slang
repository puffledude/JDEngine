

struct VSInput {
    // Slang will automatically assign Vulkan locations 0, 1, and 2 based on order
    float3 inPos;
    float3 inColor;
    float2 inTexCoord;
};

struct VSOutput
{
    float2 texCoord;

};

[Shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    output.texCoord = input.inTexCoord;
    return output;
}

[[vk::binding(0, 0)]]
Sampler2D deferredOut;

[[vk::binding(1, 0)]]
Sampler2D skyboxOut;

[Shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_Target
{
    // Sample the deferred output and skybox output
    float4 deferredColor = deferredOut.Sample(vertIn.texCoord);
    float4 skyboxColor = skyboxOut.Sample(vertIn.texCoord);
    // Simple blending: If the deferred output has alpha > 0, use it; otherwise, use the skybox color
    return (deferredColor.a > 0.0) ? deferredColor : skyboxColor;
}