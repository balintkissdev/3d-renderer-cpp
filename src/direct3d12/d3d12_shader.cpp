#include "d3d12_shader.hpp"

#include "utils.hpp"

#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <filesystem>

using namespace winrt;
namespace fs = std::filesystem;

namespace D3D12Shader
{
bool Compile(const Params& param,
             const ShaderCompileType shaderType,
             com_ptr<ID3DBlob>& shaderOut)
{
    com_ptr<ID3DBlob> error;
    constexpr UINT compileFlags =
#if defined(DEBUG) || defined(_DEBUG)
        // Skipping optimizations enable debugging with tools
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        0
#endif
    ;

    fs::path shaderPath
        = fs::path("assets/shaders/hlsl/") / param.shaderBaseName;
    shaderPath.concat(".hlsl");
    const char* entryFunction
        = shaderType == ShaderCompileType::VertexShader ? "main_vs" : "main_ps";
    const char* shaderModel
        = shaderType == ShaderCompileType::VertexShader ? "vs_5_0" : "ps_5_0";

    std::vector<D3D_SHADER_MACRO> macros;
    macros.reserve(param.defines.size() + 1);
    for (const std::string& d : param.defines)
    {
        macros.push_back(D3D_SHADER_MACRO{d.c_str(), "1"});
    }
    // NULL terminator because of course D3DCompileFromFile() expects C array
    macros.push_back(D3D_SHADER_MACRO{nullptr, nullptr});

    // UWP apps don't support D3DCompileFromFile() outside of development
    HRESULT hr = D3DCompileFromFile(shaderPath.c_str(),
                                    macros.data(),
                                    D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                    entryFunction,
                                    shaderModel,
                                    compileFlags,
                                    0,
                                    shaderOut.put(),
                                    error.put());
    if (FAILED(hr))
    {
        const std::string errorCause
            = error ? static_cast<const char*>(error->GetBufferPointer())
                    : "unknown error";
        utils::showErrorMessage("unable to compile HLSL shader",
                                shaderPath.string(),
                                "for Direct3D 12:",
                                errorCause);
        return false;
    }

    return true;
}
}  // namespace D3D12Shader
