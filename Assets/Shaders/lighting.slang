
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
    float4x4 jitteredProjection;
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
    output.pos = float4(input.inPos, 1.0);
    output.color = input.inColor;
    output.texCoord = input.inTexCoord;

    return output;
}

struct lightTransmition {
    float4 position;
    float4 direction;
    float4 colour;
    float4 radius;
};

[[vk::binding(1, 0)]]
StructuredBuffer<lightTransmition> lightTransmitions;

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
    float3 normal = normalize(gNormal.Sample(vertIn.texCoord).xyz * 2.0-1.0);
    float4 material = gMaterial.Sample(vertIn.texCoord);
    float4 fragPos = gPosition.Sample(vertIn.texCoord);

    float roughness = material.r;
    float metallic = material.g;
    float3 albedo = colour.rgb;

    float depth = shadowMap.Sample(vertIn.texCoord).r;
    // Discard or return black if hitting the background/sky (no geometry was rendered here)
    if (colour.a < 0.01) {
        return float4(0.0, 0.0, 0.0, 0.0);
    }
    float3 viewDir = normalize(cameraData.position.xyz - fragPos.xyz);

    float4x4 lightSpaceMatrix = mul(lightCameraData.jitteredProjection, lightCameraData.view);
    float4 fragPosLightSpace = mul(lightSpaceMatrix, float4(fragPos.xyz, 1.0));
    int numLights = lightNum.lightCount;
    float3 totalLighting = albedo *0.1;
    
    for (uint i = 0; i < numLights; i++) {
        lightTransmition light = lightTransmitions[i];
        light.radius = max(light.radius, 0.001); // Prevent division by zero
        float distance = length(light.position.xyz - fragPos.xyz);
        if (distance > light.radius.x) {
            continue; // Skip lights that are too far away
        }
        float falloff = 1.0 - (distance / light.radius.x);


        float3 lightDir = normalize(light.position.xyz - fragPos.xyz);

        float diff = max(dot(normal, lightDir), 0.0);
        float3 diffuse = diff * light.colour.rgb * albedo;

        float3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfwayDir), 0.0), 16.0);
        float3 specular = spec * light.colour.rgb * (1.0 - roughness);

        float shadow = 0.0;
        if (i == 0) { // Shadow only cast for the first light.
            float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
            shadow = shadowCalculation(fragPosLightSpace, bias);
        }
        totalLighting += (1.0 - shadow) * (diffuse + specular) * falloff;
    }
    return float4(totalLighting, 1.0);
}
