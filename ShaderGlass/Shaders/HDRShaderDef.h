/*
ShaderGlass: shader effect overlay
Copyright (C) 2021-2025 mausimus (mausimus.net)
https://github.com
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

        // UBO layout matching RetroArch Nightly hdr_sm5.hlsl.h (16-byte aligned)
        Params.push_back(ShaderParam("MVP", 0, 0, 64, 0.0f, 0.0f, 0.0f));
        Params.push_back(ShaderParam("HDRPaperWhite", 0, 64, 4, 80.0f, 1000.0f, 200.0f, 10.0f, "Brightness"));
        Params.push_back(ShaderParam("HDRMaxNits", 0, 68, 4, 100.0f, 4000.0f, 1000.0f, 50.0f, "Peak Brightness"));
        Params.push_back(ShaderParam("HDRExpandGamut", 0, 72, 4, 0.0f, 3.0f, 0.0f, 1.0f, "Colour Boost (0=Accurate 1=Exp 2=Wide 3=Super)"));
        Params.push_back(ShaderParam("HDRInverseTonemap", 0, 76, 4, 0.0f, 1.0f, 1.0f, 1.0f));
        Params.push_back(ShaderParam("HDR10", 0, 80, 4, 0.0f, 1.0f, 1.0f, 1.0f));
        Params.push_back(ShaderParam("HDRUseDisplayPeak", 0, 84, 4, 0.0f, 1.0f, 1.0f, 1.0f, "Use Display Peak"));
        Samplers.push_back(ShaderSampler("Source", 2));

        VertexSource = R"(
cbuffer UBO : register(b0)
{
    row_major float4x4 global_MVP : packoffset(c0);
    float global_HDRPaperWhite : packoffset(c4.x);
    float global_HDRMaxNits : packoffset(c4.y);
    float global_HDRExpandGamut : packoffset(c4.z);
    float global_HDRInverseTonemap : packoffset(c4.w);
    float global_HDR10 : packoffset(c5.x);
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
    float global_HDRPaperWhite : packoffset(c4.x);
    float global_HDRMaxNits : packoffset(c4.y);
    float global_HDRExpandGamut : packoffset(c4.z);
    float global_HDRInverseTonemap : packoffset(c4.w);
    float global_HDR10 : packoffset(c5.x);
};

Texture2D<float4> Source : register(t2);
SamplerState _Source_sampler : register(s2);

static const float kMaxNitsFor2084 = 10000.0f;

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

static const float3x3 kP3to2020 =
{
    { 0.753833f,  0.198597f,  0.047570f },
    { 0.045744f,  0.941777f,  0.012479f },
    { -0.001210f, 0.017602f,  0.983609f }
};

float3 LinearToST2084(float3 normalizedLinearValue)
{
    float3 ST2084 = pow((0.8359375f + 18.8515625f * pow(abs(normalizedLinearValue), 0.1593017578f)) / (1.0f + 18.6875f * pow(abs(normalizedLinearValue), 0.1593017578f)), 78.84375f);
    return ST2084;
}

float3 HDRDecodeGamma(float3 value)
{
    return sign(value) * pow(abs(value), 2.4f);
}

float3 To2020(const float3 sdr_linear)
{
    float3 result;

    if(global_HDRExpandGamut < 0.5f)
    {
        result = mul(k709to2020, sdr_linear);
    }
    else if(global_HDRExpandGamut < 1.5f)
    {
        result = mul(kExpanded709to2020, sdr_linear);
    }
    else if(global_HDRExpandGamut < 2.5f)
    {
        result = mul(kP3to2020, sdr_linear);
    }
    else
    {
        result = sdr_linear;
    }

    return max(result, float3(0.0f, 0.0f, 0.0f));
}

float3 InverseTonemap(const float3 sdr_linear, const float max_nits, const float brightness_nits)
{
    const float input_val = max(sdr_linear.r, max(sdr_linear.g, sdr_linear.b));

    if(input_val < 0.0001f)
        return sdr_linear;

    const float peak_ratio     = max_nits / brightness_nits;
    const float numerator      = input_val;
    const float denominator    = 1.0f - input_val * (1.0f - (1.0f / peak_ratio));
    const float tonemapped_val = numerator / max(denominator, 0.0001f);

    return sdr_linear * (tonemapped_val / input_val);
}

float3 HDR(const float3 sdr_linear)
{
    return InverseTonemap(sdr_linear, global_HDRPaperWhite, global_HDRPaperWhite);
}

float3 HDR10(const float3 hdr_linear)
{
    const float3 pq_input = hdr_linear * (global_HDRPaperWhite / kMaxNitsFor2084);
    return LinearToST2084(max(pq_input, 0.0f));
}

float3 Hdr(float3 sdr)
{
    if((global_HDRInverseTonemap > 0.0f) && (global_HDR10 > 0.0f))
    {
        return HDR10(HDR(To2020(HDRDecodeGamma(sdr))));
    }
    else if(global_HDRInverseTonemap > 0.0f)
    {
        return HDR(To2020(HDRDecodeGamma(sdr)));
    }
    else if(global_HDR10 > 0.0f)
    {
        return HDR10(To2020(HDRDecodeGamma(sdr)));
    }
    else
    {
        return sdr;
    }
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
    stage_output.FragColor = float4(Hdr(source.rgb), 1.0f);
    return stage_output;
}
)";
    }
};
