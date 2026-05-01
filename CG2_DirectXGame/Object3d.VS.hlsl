struct VertexShaderOutput
{
    float32_t4 position : SV_Position;
};

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
};

VertexShaderOutput main(VertexShaderInput intput)
{
    VertexShaderOutput output;
    output.position = intput.position;
    return output;
}