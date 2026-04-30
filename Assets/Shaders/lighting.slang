
// Shader adpated from:
//  LearnOpengl ShadowMapping https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping  Accessed 30.04.2026
//  LearnOpengl deferred shading https://learnopengl.com/Advanced-Lighting/Deferred-Shading Accessed 30.04.2026
//  LearnOpengl PBR https://learnopengl.com/PBR/Theory Accessed 30.04.2026


struct VSInput {
    float3 inPos;
    float3 inColor;
    float2 inTexCoord;
};




struct CameraInfo {
    float4 position;
    float4x4 view;
    float4x4 projection;
};
[[vk::binding(0, 0)]]
ConstantBuffer<CameraInfo> cameraData;

struct PushConstants {
    uint lightCount;
};


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

struct lightTransmition {
    float4 position;
    float4 direction;
    float4 colour;
};

[[vk::binding(1, 0)]]
ConstantBuffer<lightTransmition> lightTransmitions;

[[vk::binding(2, 0)]]
ConstantBuffer<CameraInfo> lightCameraData;

[[vk::binding(3, 0)]]
Sampler2D shadowMap;
[[vk::binding(4, 0)]]
Sampler2D gColour;
[[vk::binding(5, 0)]]
Sampler2D gNormal;
[[vk::binding(6, 0)]]
Sampler2D gMaterial;
[[vk::binding(7, 0)]]
Sampler2D gPosition;




float shadowCalculation(float4 fragposLightSpace, float bias)
{
    float3 projCoords = fragposLightSpace.xyz / fragposLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;                    // Transform to [0,1] range
    float closestDepth = shadowMap.Sample(projCoords.xy).r; // Depth from shadow map
    float currentDepth = projCoords.z;                      // Current fragment depth

    //float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0; // Simple bias to prevent shadow acne
    return shadow;

};

[Shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_Target
{
    float4 colour = gColour.Sample(vertIn.texCoord);
    float3 normal = normalize(gNormal.Sample(vertIn.texCoord).xyz);
    float4 material = gMaterial.Sample(vertIn.texCoord);
    float4 fragPos = gPosition.Sample(vertIn.texCoord);

    float roughness = material.r;
    float metallic = material.g;
    float3 albedo = colour.rgb;

    float depth = shadowMap.Sample(vertIn.texCoord).r;
    // Discard or return black if hitting the background/sky (no geometry was rendered here)
    if (colour.a < 0.01) {
        return float4(0.0, 0.0, 0.0, 1.0);
    }
    float3 viewDir = normalize(cameraData.position.xyz - fragPos.xyz);
    float3 lightDir = normalize(lightTransmitions.position.xyz - fragPos.xyz);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    float3 halfDir = normalize(viewDir + lightDir);
    float specular = pow(max(dot(normal, halfDir), 0.0), 16.0); // Simple Blinn-Phong specular term
    float shadow = shadowCalculation(fragPos, bias);                  // Calculate shadow factor
    float4x4 lightSpaceMatrix = mul(lightCameraData.projection, lightCameraData.view);


    float3 lighting = albedo * 0.1; // Ambient term
    return float4(lighting, 1.0);

}