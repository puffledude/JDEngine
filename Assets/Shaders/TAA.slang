//Placeholder shader for TAA, not used yet

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

[[vk::binding(2, 0)]]
Sampler2D newFrame;

[[vk::binding(3, 0)]]
Sampler2D newFrameDepth;

[[vk::binding(4, 0)]]
Sampler2D velocityBuffer;

[[vk::binding(5, 0)]]
Sampler2D historyFrame;

[[vk::binding(6, 0)]]
Sampler2D historyDepth;

//Stages:
/*
1. Jitter. Handled in the g buffer.
2. Generate velocity buffer. Movement of camera plus object movement. TODO.
3. Reproject using velocity. Go through each pixel, use the velocity to find the corresponding pixel in the history frame, and fetch that colour. TODO
4. Neightborhood clamping. For those we rejected, take the surrounding pixels and find the closest depth match, and use that colour instead. TODO
5. Blending. Blend the reprojected colour with the new frame colour, using a fixed alpha for now.
*/

/*
Notes here. Use previous camera data to calculate the position in the previous frame.
Get the world space position by sampling the depth buffer, then reproject into previous frame using their vp matrix (might be inverse)
Obtain the NDC coords.
Useing velocity buffer, do the same but can do so for dynamic scenes as well.
Essentially rewinding to previous postion.
*/

float3 generateWorldSpacePos(float2 texCoord, CameraInfo data) {
    // Depth between 0 and 1 as vulkan. Need to convert to world space using the camera data.
    float depth = newFrameDepth.Sample(texCoord).r;                                  // Depth mesured between 0 and 1
    float4 ndc = float4(texCoord.x * 2 - 1, texCoord.y * 2 - 1, depth, 1.0);         // Convert to NDC space
    float4 viewSpacePos = mul(data.inverseProjection, ndc);                          // Convert to view space
    viewSpacePos /= viewSpacePos.w;                                                  // Perspective divide
    float4 worldSpacePos = mul(data.inverseView, viewSpacePos);                      // Convert to world space
    return worldSpacePos.xyz;
    // distance between the near and far plane * depth + near plane distance
}



[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_Target
{
    float alpha = 0.1;
    float depthRejectionvalue = 0.1;

    float3 worldSpacePos = generateWorldSpacePos(vertIn.texCoord, cameraData);
    float4 oldPos = mul(prevCameraData.jitteredProjection, mul(prevCameraData.view, float4(worldSpacePos, 1.0)));
    float2 reprojectPos = 0.5 * (oldPos.xy / oldPos.w) + 0.5;

    float2 velocity = reprojectPos - vertIn.texCoord;

    float2 velocityPos = reprojectPos - velocity;

    // float2 adjustedReprojectPos = reprojectPos + velocityBuffer.Sample(vertIn.texCoord).xy; // Adjust the reprojected position using the velocity buffer
    float4 historySample = historyFrame.Sample(reprojectPos);
    float4 newDepth = newFrameDepth.Sample(velocityPos);
    float4 newSample = newFrame.Sample(velocityPos);

    float4 historyDepthSample = historyDepth.Sample(reprojectPos);


    if (abs(newDepth.r - historyDepthSample.r) > depthRejectionvalue)
    {
        alpha = 1;
    }

    float4 colour = alpha * newSample + (1 - alpha) * historySample;

    return colour;
}