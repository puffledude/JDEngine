//Placeholder shader for TAA, not used yet

struct CameraInfo {
    float4 position;
    float4x4 view;
    float4x4 projection;
    float4x4 jitteredProjection;
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
Sampler2D newFrame;

[[vk::binding(2, 0)]]
Sampler2D newFrameDepth;

[[vk::binding(3, 0)]]
Sampler2D velocityBuffer;

[[vk::binding(4, 0)]]
Sampler2D historyFrame;

[[vk::binding(5, 0)]]
Sampler2D historyDepth;


[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_Target
{
    float alpha = 0.1;
    float depthRejectionvalue = 0.1;

    float4 newSample = newFrame.Sample(vertIn.texCoord);
    float4 historySample = historyFrame.Sample(vertIn.texCoord);
    float4 newDepth = newFrameDepth.Sample(vertIn.texCoord);
    float4 historyDepthSample = historyDepth.Sample(vertIn.texCoord);
    if (abs(newDepth.r - historyDepthSample.r) > depthRejectionvalue)
    {
        return newSample;
    }

    float4 colour = alpha * newSample + (1 - alpha) * historySample;

    return colour;
}