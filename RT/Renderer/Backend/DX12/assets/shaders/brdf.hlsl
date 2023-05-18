#ifndef BRDF_HLSL
#define BRDF_HLSL

// ------------------------------------------------------------------
// BRDF

struct MaterialDesc
{
    float ior;
    float roughness;
    float metallic;
    float3 albedo;
};

struct BRDFDataON
{
    float roughness;

    float3 v; // Direction to viewer
    float3 n; // Surface normal
    float3 h; // Half vector
    float3 l; // Direction to light
};

float OrenNayar(BRDFDataON data)
{
    float LdotV = dot(data.l, data.v);
    float NdotL = dot(data.l, data.n);
    float NdotV = dot(data.n, data.v);

    float s = LdotV - NdotL * NdotV;

    float sigma2 = data.roughness * data.roughness;
    float A = 1.0 - 0.5 * (sigma2 / (((sigma2 + 0.33) + 0.000001)));
    float B = 0.45 * sigma2 / ((sigma2 + 0.09) + 0.00001);

    float ga = dot(data.v - data.n * NdotV, data.n - data.n * NdotL);

    return max(0.0, NdotL) * max(0, (A + B * max(0.0, ga) * sqrt(max((1.0 - NdotV * NdotV) * (1.0 - NdotL * NdotL), 0.0)) / max(NdotL, NdotV)));
}

float3 EvaluateOrenNayar(BRDFDataON data)
{
    float oren_nayar = OrenNayar(data);
    return float3(oren_nayar, oren_nayar, oren_nayar);
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    float F = pow(saturate(1.0 - cosTheta), 5.0);
    return F + F0 * (1.0 - F);
}

float DistributionGGX(float NoH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NoH2 = NoH * NoH;

    float num = a2;
    float denom = (NoH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

// TODO(daniel): Do my due diligence working through the theory here

float G1_Smith(float NoV, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float NoV2 = NoV * NoV;

    float num = 2.0 * NoV;
    float denom = NoV + sqrt(a2 + (1.0 - a2) * NoV2);

    return num / denom;
}

float G1_Schlick(float NoV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NoV;
    float denom = NoV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(float NoV, float NoL, float roughness)
{
    // NOTE(daniel): To match the direct GGX sampling routine, G1_Smith is used as the
    // shadowing function instead of G1_Schlick
    // float ggx1 = G1_Schlick(NoV, roughness);
    // float ggx2 = G1_Schlick(NoL, roughness);
    float ggx1 = G1_Smith(NoV, roughness);
    float ggx2 = G1_Smith(NoL, roughness);

    return ggx1 * ggx2;
}

float3 EvaluateBRDFSpecular(float NoV, float NoH, float NoL, float3 F, MaterialDesc material)
{
    float D = DistributionGGX(NoH, material.roughness);
    float G2 = GeometrySmith(NoV, NoL, material.roughness);

    float3 numerator = F * D * G2;
    float denominator = 4.0 * NoV * NoL + 0.0001;
    float3 specular = numerator / denominator;

    return specular;
}

float3 EvaluateBRDFDiffuse(float3 F, MaterialDesc material)
{
    // TODO: Oren-Nayar diffuse?
    float3 diffuse = (1.0 - F) * (1.0 - material.metallic);
    return diffuse;
}

void EvaluateBRDF(float3 V, float3 N, float3 L, MaterialDesc material, out float3 diffuse, out float3 specular)
{
    if (tweak.enable_pbr)
    {
        float3 H = normalize(V + L);

        float NoV = max(0, dot(N, V));
        float NoH = max(0, dot(N, H));
        float NoL = max(0, dot(N, L));
        float HoV = max(0, dot(H, V));

        float3 F0 = lerp(material.ior, material.albedo, material.metallic);
        float3 F = FresnelSchlick(HoV, F0);

        specular = EvaluateBRDFSpecular(NoV, NoH, NoL, F, material);

        if (tweak.use_oren_nayar_brdf)
        {
            BRDFDataON data;
            data.roughness = material.roughness;
            data.v = V;
            data.n = N;
            data.h = H;
            data.l = L;
            diffuse = (EvaluateOrenNayar(data) * (1.0 - material.metallic)) / PI;
        }
        else
        {
            diffuse = EvaluateBRDFDiffuse(F, material) / PI;
        }
    }
    else
    {
        diffuse = 1.0 / PI;
        specular = 0;
    }
}

float3 SampleGGXVNDF(float3 Ve, float roughness, float r1, float r2)
{
    // SOURCE: Sampling the GGX Distribution of Visible Normalz, Eric Heitz 2018

    // NOTE: This code assumes Z = up, but for us Y = up, so swizzle
    Ve = Ve.xzy;

    float alpha = roughness * roughness;

    float3 Vh = normalize(float3(alpha * Ve.x, alpha * Ve.y, Ve.z));

    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    float3 T1 = lensq > 0 ? float3(-Vh.y, Vh.x, 0) * rsqrt(lensq) : float3(1, 0, 0);
    float3 T2 = cross(Vh, T1);

    float r = sqrt(r1);
    float phi = 2.0 * PI * r2;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5 * (1.0 + Vh.z);
    t2 = (1.0 - s) * sqrt(1.0 - t1 * t1) + s * t2;

    float3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0, 1.0 - t1 * t1 - t2 * t2)) * Vh;

    float3 Ne = normalize(float3(alpha * Nh.x, alpha * Nh.y, max(0, Nh.z)));

    // And swizzle back when we're done
    return Ne.xzy;
}

#endif /* BRDF_HLSL */
