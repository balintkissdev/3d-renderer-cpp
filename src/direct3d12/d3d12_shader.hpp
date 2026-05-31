#ifndef D3D12_SHADER_HPP_
#define D3D12_SHADER_HPP_

#include <d3d12.h>
#include <string>
#include <vector>
#include <winrt/base.h>

namespace D3D12Shader
{
struct Params
{
    std::string shaderBaseName;
    std::vector<std::string> defines;
};

enum class ShaderCompileType : uint8_t
{
    VertexShader,
    PixelShader,
};

bool Compile(const Params& param,
             const ShaderCompileType shaderType,
             winrt::com_ptr<ID3DBlob>& shaderOut);

};  // namespace D3D12Shader

#endif
