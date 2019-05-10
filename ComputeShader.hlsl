
struct InputSR
{
    float a;
    int b;
};
cbuffer InputCB : register(b0)
{
    float c;
    int d;
    int padding1;
    int padding2;
};
struct Output
{
    float r;
};

RWStructuredBuffer<Output> ShaderOutput : register(u0);
StructuredBuffer<InputSR> ShaderInput : register(t0);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    float outputValue = float(ShaderInput[DTid.x].a * c) + float(ShaderInput[DTid.x].b * d);
    ShaderOutput[DTid.x].r = outputValue;
}
