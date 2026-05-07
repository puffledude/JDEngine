//Placeholder to compile
struct VSInput {
    float3 inPos;
    float3 inColor;
    float2 inTexCoord;
};

[[vk::binding(0, 0)]]
struct CameraInfo {
    float4 position;
    float4x4 view;
    float4x4 projection;
    float4x4 jitteredProjection;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float3 color;
    float2 texCoord;
}

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    output.pos = float4(input.inPos, 1.0);
    output.color = input.inColor;
    output.texCoord = input.inTexCoord;

    return output;
}

[[vk::binding(1, 0)]]
Sampler2D taaImage;

[[vk::binding(2, 0)]]
Sampler2D laplaceImage;

// Shader idea from https://youtu.be/VbJK0UyZMXU
// Also use this https://www.youtube.com/watch?v=uNP6ZwQ3r6A
//Not sure how well this will work but might.

//Apply the results of the laplace filter.

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_Target
{
    return taaImage.Sample(vertIn.texCoord);
}