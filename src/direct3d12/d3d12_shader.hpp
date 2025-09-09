#ifndef D3D12_PSO_
#define D3D12_PSO_

#include <d3d12.h>
#include <winrt/base.h>

namespace D3D12Shader
{

enum class ShaderCompileType : uint8_t
{
    VertexShader,
    PixelShader,
};

bool CompileShader(std::string_view hlslFilename,
                   const ShaderCompileType shaderType,
                   winrt::com_ptr<ID3DBlob>& shaderOut);

};  // namespace D3D12Shader

#endif
