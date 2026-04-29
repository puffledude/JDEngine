struct VSInput {
    float3 inPos;
    float3 inColor;
    float2 inTexCoord;
};

struct lightTransmition {
    float3 position;
    float3 direction;
    float3 colour;
    float4 luminosity;
};
[[vk::binding(0, 0)]]
ConstantBuffer<lightTransmition> lightTransmitions;

struct CameraInfo {
    float4 position;
    float4x4 view;
    float4x4 projection;
};
[[vk::binding(1, 0)]]
ConstantBuffer<CameraInfo> cameraData;

//[[vk::push_constant]]
//ConstantBuffer<PushConstants> pc;


struct VSOutput
{
    float4 pos : SV_Position;
    float3 color;
    float2 texCoord;
}

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
  //  output.pos = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(input.inPos,1.0))));
    //output.color = input.inColor;
    //output.texCoord = input.inTexCoord;
    return output;
}



[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_Target
{
    float4 albedoData = gColour.Sample(vertIn.texCoord);
    float3 normal = normalize(gNormal.Sample(vertIn.texCoord).xyz);
    float4 material = gMaterial.Sample(vertIn.texCoord);
    float4 fragPos = gPosition.Sample(vertIn.texCoord);

    float roughness = material.r;
    float metallic = material.g;
    float3 albedo = albedoData.rgb;

    // Discard or return black if hitting the background/sky (no geometry was rendered here)
    if (albedoData.a < 0.01) {
        return float4(0.0, 0.0, 0.0, 1.0);
    }

    // 2. Base Setup
    float3 viewDir = normalize(cameraData.position - fragPos.xyz);

    // A small amount of ambient light so shadows aren't 100% black
    float3 totalLighting = albedo * 0.05;

    // 3. Accumulate Light
    for (uint i = 0; i < pc.lightCount; ++i) {
        Light light = sceneLights[i];

        float3 lightDir = normalize(light.position - fragPos.xyz);
        float distance = length(light.position - fragPos.xyz);

        // Attenuation (Inverse Square Law)
        // Add 1.0 to denominator to prevent division by zero when very close to the light source
        float attenuation = 1.0 / (1.0 + (distance * distance));

        // Radiance
        float3 radiance = light.colour * light.luminosity * attenuation;

        // Diffuse factor
        float NdotL = max(dot(normal, lightDir), 0.0);
        float3 diffuse = albedo * NdotL;

        // Specular factor (Blinn-Phong approximation for PBR-lite)
        float3 halfwayDir = normalize(lightDir + viewDir);
        float NdotH = max(dot(normal, halfwayDir), 0.0);

        // Modulate shininess based on roughness
        float shininess = (1.0 - roughness) * 128.0;
        float spec = pow(NdotH, max(shininess, 0.001)); // max prevents pow(0,0)

        // F0: Metals tint specular with albedo, non-metals use a base 0.04
        float3 f0 = lerp(float3(0.04), albedo, metallic);
        float3 specular = f0 * spec;

        // Add this light to our total
        totalLighting += (diffuse + specular) * radiance;
    }

    // Return the final accumulated lighting (Alpha is set to 1.0)
    return float4(totalLighting, 1.0);
}