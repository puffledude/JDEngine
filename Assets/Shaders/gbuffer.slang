
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

[Shader("vertex")]
VSOutput vertMain(VSInput input) {

    VSOutput output;
    output.pos = float4(input.inPos, 1.0);
    output.color = input.inColor;
    output.texCoord = input.inTexCoord;
    return output;
}

Sampler2D colourTexure;
Sampler2D normalTexture;

[Shader("fragment")]
float4 fragMain(VSOutput vertIn): SV_Target
{
    //float4 color = tex2D(colourTexure, vertIn.texCoord);
    // float3 normal = tex2D(normalTexture, vertIn.texCoord).rgb;
    return colourTexure.Sample(vertIn.texCoord);
}