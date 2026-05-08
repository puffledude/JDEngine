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
};

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

float4 generateHistoryColour(float2 prevTexCoord, float2 currentTexCoord, float2 texelSize, inout float clampMultipler) {
    // Using min and max to clamp the colour to the surrounding pixels.
    // Taking inspiration from https://www.elopezr.com/temporal-aa-and-the-quest-for-the-holy-trail/
    float4 prevColour = historyFrame.Sample(prevTexCoord);
    float4 minColour = float4(9999);
    float4 maxColour = float4(-9999);
    
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float4 colour = newFrame.Sample(currentTexCoord + float2(x, y) * texelSize);
            minColour = min(minColour, colour);
            maxColour = max(maxColour, colour);
            if (colour.equals(minColour) || colour.equals(maxColour)) {
                clampMultipler *= 0.5;
            }
        }
    }
    float3 clampedColour = clamp(prevColour, minColour, maxColour).rgb;
    return float4(clampedColour, prevColour.a);
}

float4 generateHistoryColourfiveTap(float2 prevTexCoord, float2 currentTexCoord, float2 texelSize)
{
    float2 xyone = float2(-1, 0) * texelSize;
    float4 colourone = historyFrame.Sample(prevTexCoord + xyone);
    float2 xytwo = float2(1, 0) * texelSize;
    float4 colourtwo = historyFrame.Sample(prevTexCoord + xytwo);
    float2 xythree = float2(0, -1) * texelSize;
    float4 colourthree = historyFrame.Sample(prevTexCoord + xythree);
    float2 xyfour = float2(0, 1) * texelSize;
    float4 colourfour = historyFrame.Sample(prevTexCoord + xyfour);
    float4 colourcenter = historyFrame.Sample(prevTexCoord);
    float4 blendedColour = (colourone + colourtwo + colourthree + colourfour + colourcenter) / 5.0;
    float3 minColour = float3(9999);
    float3 maxColour = float3(-9999);
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float3 colour = newFrame.Sample(currentTexCoord + float2(x, y) * texelSize).rgb;
            minColour = min(minColour, colour);
            maxColour = max(maxColour, colour);
        }
    }
    float3 clampedColour = clamp(blendedColour.rgb, minColour, maxColour);
    return float4(clampedColour, colourcenter.a);
}

float2 calculateVelocity(float2 currentTexCoord, float2 texelSize) {
    // Use nearnest depth fragment within 3x3 region.
    float minDepth = 9999;
    float2 closestTexCoord = currentTexCoord;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float depth = newFrameDepth.Sample(currentTexCoord + float2(x, y) * texelSize).r;
            if (depth < minDepth) {
                minDepth = depth;
                closestTexCoord = currentTexCoord + float2(x, y) * texelSize;
            }
        }
    }
    float2 velocity = velocityBuffer.Sample(closestTexCoord).xy;

    return velocity;
}

float lineariseDepth(float depth, float near, float far)
{
    return (near * far) / (far - depth * (far - near));
}



[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_Target
{
    //float alpha = 0.1;
    float depthRejectionvalue = 0.1;
   /// float3 worldSpacePos = generateWorldSpacePos(vertIn.texCoord, cameraData);
    //float4 oldPos = mul(prevCameraData.jitteredProjection, mul(prevCameraData.view, float4(worldSpacePos, 1.0)));
    //float2 reprojectPos = 0.5 * (oldPos.xy / oldPos.w) + 0.5;


    float2 currentPos = vertIn.texCoord;
    uint velWidth;
    uint velHeight;
    velocityBuffer.GetDimensions(velWidth, velHeight);
    float2 veltexelSize = float2(1.0 / velWidth, 1.0 / velHeight);

    float2 velocity = calculateVelocity(currentPos, veltexelSize);

    float2 reprojectPos = currentPos - velocity; // Reproject the current position back to the previous frame using the velocity buffer

    uint width=0;
    uint height=0;
    uint mipLevel=0;
    uint levelNum = 0;

    historyFrame.GetDimensions(mipLevel, width, height, levelNum);
    float texSize = width * height;
    float2 texelSize = float2(1.0 / width, 1.0 / height);

    float alphaClamMultiplier = 1.0;
    float4 historySample = generateHistoryColour(reprojectPos, currentPos, texelSize, alphaClamMultiplier);
    //float4 historySample = historyFrame.Sample(reprojectPos);
    float4 newSample = newFrame.Sample(currentPos);

    //Local Clamping next. Need to clamp or clip the colour to between the colours of the surrounding pixels.
    //[Sousa13] gives implementation idea.

    float sampleCount = historySample.a;
    float alpha = max(0.1, 1 / sampleCount);
    float4 newDepth = newFrameDepth.Sample(currentPos);

    float near = cameraData.projection[3][2] / (cameraData.projection[2][2] - 1.0);
    float far = cameraData.projection[3][2] / (cameraData.projection[2][2] + 1.0);

    float depthLinear = lineariseDepth(newDepth.r, near, far); // Assuming near plane at 0.1 and far plane at 100. Adjust as needed.
    float4 historyDepthSample = historyDepth.Sample(reprojectPos);
    

    if (abs(depthLinear - lineariseDepth(historyDepthSample.r, near, far)) > depthRejectionvalue)
    {
        alpha = 1;
        sampleCount = 1;
    }
    float totalVelocity = length(velocity);
    float4 colour = float4(0.0);
    if (totalVelocity > 2) { //Attempt at motion blurring.
        float decimal = 2 / totalVelocity;
        // float lerpFactor = pow(decimal, 2); // Adjust the exponent to control the falloff
        float lerpFactor = 1 - decimal;
        colour = lerp(newSample, historySample, lerpFactor);
        colour.a += 1; // Set alpha to 1 to indicate a valid sample
        }
    else {
        // alpha = 0.1;
            alpha *= alphaClamMultiplier;
            colour = alpha * newSample + (1 - alpha) * historySample;
            colour.a = sampleCount + 1;
    }
    return colour;
}