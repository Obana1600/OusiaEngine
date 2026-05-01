struct PixelShaderPutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderPutput main()
{
    PixelShaderPutput output;
    output.color = float32_t4(1.0, 1.0, 1.0, 1.0);
    return output;
}