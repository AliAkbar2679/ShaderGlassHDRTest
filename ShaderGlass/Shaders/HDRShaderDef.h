/*
ShaderGlass: shader effect overlay
Copyright (C) 2021-2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderGlass
GNU General Public License v3.0
*/

#pragma once

class HDRShaderDef : public ShaderDef
{
public:
    HDRShaderDef() : ShaderDef {}
    {
        Name           = "hdr_output";
        VertexLength   = 0;
        FragmentLength = 0;

        // UBO layout matching RetroArch dxgi_hdr_uniform_t (16-byte aligned)
        Params.push_back(ShaderParam("MVP", 0, 0, 64, 0.0f, 0.0f, 0.0f));
        Params.push_back(ShaderParam("HDRContrast", 0, 64, 4, 0.0f, 10.0f, 5.0f, 0.1f, "HDR Contrast"));
        Params.push_back(ShaderParam("HDRPaperWhite", 0, 68, 4, 80.0f, 1000.0f, 200.0f, 10.0f, "HDR Paper White Luminance"));
        Params.push_back(ShaderParam("HDRMaxNits", 0, 72, 4, 100.0f, 4000.0f, 1000.0f, 50.0f, "HDR Peak Luminance"));
        Params.push_back(ShaderParam("HDRExpandGamut", 0, 76, 4, 0.0f, 1.0f, 1.0f, 1.0f, "HDR Expand Gamut"));
        Params.push_back(ShaderParam("HDRInverseTonemap", 0, 80, 4, 0.0f, 1.0f, 1.0f, 1.0f));
        Params.push_back(ShaderParam("HDR10", 0, 84, 4, 0.0f, 1.0f, 1.0f, 1.0f));
        Samplers.push_back(ShaderSampler("Source", 2));

        VertexSource = R"(
cbuffer UBO : register(b0)
{
    row_major float4x4 global_MVP : packoffset(c0);
    float global_HDRContrast : packoffset(c4.x);
    float global_HDRPaperWhite : packoffset(c4.y);
    float global_HDRMaxNits : packoffset(c4.z);
    float global_HDRExpandGamut : packoffset(c4.w);
    float global_HDRInverseTonemap : packoffset(c5.x);
    float global_HDR10 : packoffset(c5.y);
};

struct SPIRV_Cross_Input
{
    float4 Position : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;
};

struct SPIRV_Cross_Output
{
    float2 vTexCoord : TEXCOORD0;
    float4 gl_Position : SV_Position;
};

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    SPIRV_Cross_Output stage_output;
    stage_output.gl_Position = mul(stage_input.Position, global_MVP);
    stage_output.vTexCoord = stage_input.TexCoord;
    return stage_output;
}
)";

        FragmentSource = R"(
cbuffer UBO : register(b0)
{
    row_major float4x4 global_MVP : packoffset(c0);
    float global_HDRContrast : packoffset(c4.x);
    float global_HDRPaperWhite : packoffset(c4.y);
    float global_HDRMaxNits : packoffset(c4.z);
    float global_HDRExpandGamut : packoffset(c4.w);
    float global_HDRInverseTonemap : packoffset(c5.x);
    float global_HDR10 : packoffset(c5.y);
};

Texture2D<float4> Source : register(t2);
SamplerState _Source_sampler : register(s2);

static const float kMaxNitsFor2084   = 10000.0f;
static const float kEpsilon          = 0.0001f;
static const float kLumaChannelRatio = 0.25f;

static const float3x3 k709to2020 =
{
    { 0.6274040f, 0.3292820f, 0.0433136f },
    { 0.0690970f, 0.9195400f, 0.0113612f },
    { 0.0163916f, 0.0880132f, 0.8955950f }
};

static const float3x3 kExpanded709to2020 =
{
    { 0.6274040f,  0.3292820f, 0.0433136f },
    { 0.0457456f,  0.941777f,  0.0124772f },
    { -0.00121055f, 0.0176041f, 0.983607f }
};

float3 LinearToST2084(float3 normalizedLinearValue)
{
    float3 ST2084 = pow((0.8359375f + 18.8515625f * pow(abs(normalizedLinearValue), 0.1593017578f)) / (1.0f + 18.6875f * pow(abs(normalizedLinearValue), 0.1593017578f)), 78.84375f);
    return ST2084;
}

float3 Hdr(float3 sdr)
{
    float3 hdr;

    if (global_HDRInverseTonemap > 0.0f)
    {
        float contrast = max(0.1f, 10.0f - global_HDRContrast);
        sdr = pow(abs(sdr), contrast / 2.2f);

        float luma = dot(sdr, float3(0.2126f, 0.7152f, 0.0722f));

        float maxValue          = (global_HDRMaxNits / global_HDRPaperWhite) + kEpsilon;
        float elbow             = maxValue / (maxValue - 1.0f);
        float offset            = 1.0f - ((0.5f * elbow) / (elbow - 0.5f));

        float hdrLumaInvTonemap = offset + ((luma * elbow) / (elbow - luma));
        float sdrLumaInvTonemap = luma / ((1.0f + kEpsilon) - luma);

        float lumaInvTonemap    = (luma > 0.5f) ? hdrLumaInvTonemap : sdrLumaInvTonemap;
        float3 perLuma          = sdr / (luma + kEpsilon) * lumaInvTonemap;

        float3 hdrInvTonemap    = offset + ((sdr * elbow) / (elbow - sdr));
        float3 sdrInvTonemap    = sdr / ((1.0f + kEpsilon) - sdr);

        float3 perChannel       = float3(sdr.x > 0.5f ? hdrInvTonemap.x : sdrInvTonemap.x,
                                         sdr.y > 0.5f ? hdrInvTonemap.y : sdrInvTonemap.y,
                                         sdr.z > 0.5f ? hdrInvTonemap.z : sdrInvTonemap.z);

        hdr = lerp(perLuma, perChannel, kLumaChannelRatio);
    }
    else
    {
        hdr = sdr;
    }

    float3 hdr10;

    if (global_HDR10 > 0.0f)
    {
        float3 rec2020 = mul(k709to2020, hdr);

        if (global_HDRExpandGamut > 0.0f)
        {
            rec2020 = mul(kExpanded709to2020, hdr);
        }

        float3 linearColour = rec2020 * (global_HDRPaperWhite / kMaxNitsFor2084);
        hdr10 = LinearToST2084(linearColour);
    }
    else
    {
        hdr10 = hdr;
    }

    return hdr10;
}

struct SPIRV_Cross_Input
{
    float2 vTexCoord : TEXCOORD0;
};

struct SPIRV_Cross_Output
{
    float4 FragColor : SV_Target0;
};

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    SPIRV_Cross_Output stage_output;
    float4 source = Source.Sample(_Source_sampler, stage_input.vTexCoord);
    stage_output.FragColor = float4(Hdr(source.rgb), source.a);
    return stage_output;
}
)";
    }
};
