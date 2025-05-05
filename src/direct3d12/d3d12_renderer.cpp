#include "d3d12_renderer.hpp"

#include "camera.hpp"
#include "drawproperties.hpp"
#include "scene.hpp"
#include "utils.hpp"

#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

#include <combaseapi.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <dxgi1_4.h>
#include <synchapi.h>

namespace fs = std::filesystem;
using namespace winrt;
using namespace DirectX;

D3D12Renderer::D3D12Renderer(Window& window,
                             const DrawProperties& drawProps,
                             const Camera& camera)
    : Renderer{window, drawProps, camera}
    , viewPort_{CD3DX12_VIEWPORT(
          0.0F,
          0.0F,
          static_cast<float>(window_.frameBufferSize().first),
          static_cast<float>(window_.frameBufferSize().second))}
    , scissorRect_{0,
                   0,
                   window_.frameBufferSize().first,
                   window_.frameBufferSize().second}
    , rtvDescriptorHeapSize_{0}
    , cbvDescriptorHeapSize_{0}
    , frameIndex_{0}
    , fenceValue_{0}
    , vsyncEnabled_{drawProps.vsyncEnabled}
{
    models_.reserve(3);
}

D3D12Renderer::~D3D12Renderer()
{
    cleanup();
}

bool D3D12Renderer::init()
{
    if (!createDevice() || !createCommandObjects() || !createDescriptorHeaps()
        || !createRTVs() || !createDSV() || !createCBVs()
        || !createRootSignature() || !createPSO() || !loadAssets())
    {
        return false;
    }

    createSyncObjects();
    // Wait for setup to complete
    waitForPreviousFrame();

    setVSyncEnabled(drawProps_.vsyncEnabled);
    return true;
}

void D3D12Renderer::cleanup()
{
    waitForPreviousFrame();
    ::CloseHandle(fenceEvent_);
}

// TODO: Add code to setup WARP device for compatibility
bool D3D12Renderer::createDevice()
{
    UINT deviceFlags = 0;

#if defined(DEBUG) || defined(_DEBUG)
    {
        com_ptr<ID3D12Debug> debugController;
        HRESULT hr
            = D3D12GetDebugInterface(IID_PPV_ARGS(debugController.put()));
        if (SUCCEEDED(hr))
        {
            debugController->EnableDebugLayer();
            deviceFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    com_ptr<IDXGIFactory4> factory;
    CreateDXGIFactory2(deviceFlags, IID_PPV_ARGS(factory.put()));

    // Get adapter
    com_ptr<IDXGIAdapter1> adapter;

    com_ptr<IDXGIFactory6> factory6;
    HRESULT hr = factory->QueryInterface(factory6.put());
    if (FAILED(hr))
    {
        utils::showErrorMessage("unable to create DXGI factory");
        return false;
    }

    hr = factory6->EnumAdapterByGpuPreference(
        0,
        DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
        IID_PPV_ARGS(adapter.put()));
    if (FAILED(hr))
    {
        utils::showErrorMessage("unable to query GPU adapter");
        return false;
    }

    DXGI_ADAPTER_DESC1 adapterDesc;
    adapter->GetDesc1(&adapterDesc);

    hr = D3D12CreateDevice(adapter.get(),
                           D3D_FEATURE_LEVEL_12_0,
                           IID_PPV_ARGS(device_.put()));
    if (FAILED(hr))
    {
        utils::showErrorMessage("unable to initialize Direct3D 12 device");
        return false;
    }

    // Create Command Queue (Swap Chain creation requires it)
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    hr = device_->CreateCommandQueue(&commandQueueDesc,
                                     IID_PPV_ARGS(commandQueue_.put()));
    if (FAILED(hr))
    {
        utils::showErrorMessage("unable to create Command Queue");
        return false;
    }

    // Create Swap Chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.BufferCount = FRAME_COUNT;
    swapChainDesc.Width = window_.frameBufferSize().first;
    swapChainDesc.Height = window_.frameBufferSize().second;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    com_ptr<IDXGISwapChain1> swapChain;
    // From Direct3D 11.1, Microsoft recommends using this instead of
    // CreateSwapChain()
    hr = factory->CreateSwapChainForHwnd(commandQueue_.get(),
                                         window_.raw(),
                                         &swapChainDesc,
                                         nullptr,
                                         nullptr,
                                         swapChain.put());
    if (FAILED(hr))
    {
        utils::showErrorMessage("unable to create Swap Chain");
        return false;
    }
    swapChain_ = swapChain.try_as<IDXGISwapChain3>();
    if (!swapChain_)
    {
        utils::showErrorMessage("unable to create Swap Chain");
        return false;
    }
    // Fullscreen transitions not supported
    factory->MakeWindowAssociation(window_.raw(), DXGI_MWA_NO_ALT_ENTER);
    frameIndex_ = swapChain_->GetCurrentBackBufferIndex();
    return true;
}

bool D3D12Renderer::createCommandObjects()
{
    // Create Command Allocator
    HRESULT hr = device_->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(commandAllocator_.put()));
    if (FAILED(hr))
    {
        utils::showErrorMessage("unable to create Command Allocator");
        return false;
    }

    // Create command list
    hr = device_->CreateCommandList(0,
                                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                                    commandAllocator_.get(),
                                    pso_.get(),
                                    IID_PPV_ARGS(commandList_.put()));
    if (FAILED(hr))
    {
        utils::showErrorMessage("unable to create Command List");
        return false;
    }
    // Change from "recording" to "closed" state for startup
    commandList_->Close();

    return true;
}

bool D3D12Renderer::createDescriptorHeaps()
{
    // Render Target View (RTV) descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.NumDescriptors = FRAME_COUNT;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(device_->CreateDescriptorHeap(
            &rtvHeapDesc,
            IID_PPV_ARGS(rtvDescriptorHeap_.put()))))
    {
        utils::showErrorMessage(
            "unable to create Render Target View (RTV) descriptor heap");
        return false;
    }
    rtvDescriptorHeapSize_ = device_->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Depth-Stencil View (DSV) descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(device_->CreateDescriptorHeap(
            &dsvHeapDesc,
            IID_PPV_ARGS(dsvDescriptorHeap_.put()))))
    {
        utils::showErrorMessage(
            "unable to create Depth-Stencil View (DSV) descriptor heap");
        return false;
    }

    // Constant Buffer View (CBV) descriptor heap
    // Used for model transform and material update. Each model gets its own
    // CBV.
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc{};
    cbvHeapDesc.NumDescriptors = MAX_MODEL_SCENE_NODE_COUNT * 2;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(device_->CreateDescriptorHeap(
            &cbvHeapDesc,
            IID_PPV_ARGS(cbvDescriptorHeap_.put()))))
    {
        utils::showErrorMessage(
            "unable to create Constant Buffer View (CBV) descriptor heap");
        return false;
    }
    cbvDescriptorHeapSize_ = device_->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Shader Resource View (SRV) descriptor heap
    // Used by ImGui to bind font texture.
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
    srvHeapDesc.NumDescriptors = 1;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(device_->CreateDescriptorHeap(
            &srvHeapDesc,
            IID_PPV_ARGS(srvDescriptorHeap_.put()))))
    {
        utils::showErrorMessage(
            "unable to create Shader Resource View (SRV) descriptor heap");
        return false;
    }

    return true;
}

bool D3D12Renderer::createRTVs()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
        rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());

    // RTV for each frame
    for (UINT i = 0; i < FRAME_COUNT; ++i)
    {
        swapChain_->GetBuffer(i, IID_PPV_ARGS(renderTargets_[i].put()));
        device_->CreateRenderTargetView(renderTargets_[i].get(),
                                        nullptr,
                                        rtvHandle);
        rtvHandle.Offset(1, rtvDescriptorHeapSize_);
    }

    return true;
}

bool D3D12Renderer::createDSV()
{
    const CD3DX12_HEAP_PROPERTIES depthStencilHeapProps(
        D3D12_HEAP_TYPE_DEFAULT);
    const auto depthStencilTextureDesc
        = CD3DX12_RESOURCE_DESC::Tex2D(DEPTH_STENCIL_FORMAT,
                                       window_.frameBufferSize().first,
                                       window_.frameBufferSize().second,
                                       1,
                                       0,
                                       1,
                                       0,
                                       D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    D3D12_CLEAR_VALUE depthOptimizedClearValue{};
    depthOptimizedClearValue.Format = DEPTH_STENCIL_FORMAT;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0F;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;
    if (FAILED(device_->CreateCommittedResource(
            &depthStencilHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &depthStencilTextureDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            IID_PPV_ARGS(depthStencil_.put()))))
    {
        utils::showErrorMessage("unable to create Depth-Stencil texture");
        return false;
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
    depthStencilViewDesc.Format = DEPTH_STENCIL_FORMAT;
    depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;
    device_->CreateDepthStencilView(
        depthStencil_.get(),
        &depthStencilViewDesc,
        dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());

    return true;
}

// TODO: Refactor duplicate code
bool D3D12Renderer::createCBVs()
{
    mvpConstantBuffers_.resize(MAX_MODEL_SCENE_NODE_COUNT);
    mvpCbvDataBegin_.resize(MAX_MODEL_SCENE_NODE_COUNT);
    materialConstantBuffers_.resize(MAX_MODEL_SCENE_NODE_COUNT);
    materialCbvDataBegin_.resize(MAX_MODEL_SCENE_NODE_COUNT);
    const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
    for (size_t i = 0; i < MAX_MODEL_SCENE_NODE_COUNT; ++i)
    {
        // Create MVP constant buffer and view
        if (!createConstantBufferAndView<MVPConstantBuffer>(
                heapProperties,
                mvpConstantBuffers_[i],
                i,
                mvpCbvDataBegin_[i]))
        {
            return false;
        }

        // Create Material constant buffer and view
        if (!createConstantBufferAndView<MaterialConstantBuffer>(
                heapProperties,
                materialConstantBuffers_[i],
                MAX_MODEL_SCENE_NODE_COUNT + i,
                materialCbvDataBegin_[i]))
        {
            return false;
        }
    }

    return true;
}

template <typename ConstantBufferType>
bool D3D12Renderer::createConstantBufferAndView(
    const CD3DX12_HEAP_PROPERTIES& heapProperties,
    winrt::com_ptr<ID3D12Resource>& constantBuffer,
    UINT cbvDescriptorHeapIndex,
    uint8_t*& cbvDataBegin)
{
    // Create Constant Buffer
    constexpr UINT constantBufferSize = sizeof(ConstantBufferType);
    const CD3DX12_RESOURCE_DESC resourceDesc
        = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);
    HRESULT hr
        = device_->CreateCommittedResource(&heapProperties,
                                           D3D12_HEAP_FLAG_NONE,
                                           &resourceDesc,
                                           D3D12_RESOURCE_STATE_GENERIC_READ,
                                           nullptr,
                                           IID_PPV_ARGS(constantBuffer.put()));
    if (FAILED(hr))
    {
        return false;
    }

    // Create Constant Buffer View (CBV)
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
    cbvDesc.BufferLocation = constantBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = constantBufferSize;
    CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(
        cbvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
        static_cast<int>(cbvDescriptorHeapIndex),
        cbvDescriptorHeapSize_);
    device_->CreateConstantBufferView(&cbvDesc, cbvHandle);

    // Map and initialize Constant Buffer
    CD3DX12_RANGE readRange(
        0,
        0);  // We do not intend to read from this resource on the CPU
    constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&cbvDataBegin));

    return true;
}

bool D3D12Renderer::createRootSignature()
{
    // Root Signature acts as "function signature" for the shaders in the
    // pipeline, defining the parameters passed for the shaders.
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData{};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device_->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE,
                                            &featureData,
                                            sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // With descriptor table of CVBs
    std::array<CD3DX12_DESCRIPTOR_RANGE1, 2> ranges;
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
                   1,
                   0,
                   0,
                   D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
                   1,
                   1,
                   0,
                   D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    // Parameters passed to the shaders are called Root Parameters
    std::array<CD3DX12_ROOT_PARAMETER1, 2> rootParameters;
    rootParameters[0].InitAsDescriptorTable(1,
                                            ranges.data(),
                                            D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsDescriptorTable(1,
                                            &ranges[1],
                                            D3D12_SHADER_VISIBILITY_PIXEL);

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags
        = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(rootParameters.size(),
                               rootParameters.data(),
                               0,
                               nullptr,
                               rootSignatureFlags);
    com_ptr<ID3DBlob> signature;
    com_ptr<ID3DBlob> error;
    D3DX12SerializeVersionedRootSignature(&rootSignatureDesc,
                                          D3D_ROOT_SIGNATURE_VERSION_1,
                                          signature.put(),
                                          error.put());
    HRESULT hr
        = device_->CreateRootSignature(0,
                                       signature->GetBufferPointer(),
                                       signature->GetBufferSize(),
                                       IID_PPV_ARGS(rootSignature_.put()));
    if (FAILED(hr))
    {
        utils::showErrorMessage("unable to create Root Signature");
        return false;
    }
    return true;
}

bool D3D12Renderer::createPSO()
{
    // Compile shaders
    // TODO: Revisit HLSL file organization when using precompiled CSO bytecode
    const fs::path hlslShaderBasePath = "assets/shaders/hlsl/";
    com_ptr<ID3DBlob> vertexShader;
    if (!compileShader(hlslShaderBasePath / "model.vs.hlsl",
                       ShaderCompileType::VertexShader,
                       vertexShader))
    {
        return false;
    }

    com_ptr<ID3DBlob> pixelShader;
    if (!compileShader(hlslShaderBasePath / "model.ps.hlsl",
                       ShaderCompileType::PixelShader,
                       pixelShader))
    {
        return false;
    }

    struct VertexSpecification
    {
        XMFLOAT3 position;
        XMFLOAT3 normal;
    };
    const std::array<D3D12_INPUT_ELEMENT_DESC, 2> inputElementDesc = {
        D3D12_INPUT_ELEMENT_DESC{"POSITION",
                                 0,
                                 DXGI_FORMAT_R32G32B32_FLOAT,
                                 0,
                                 offsetof(VertexSpecification, position),
                                 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                 0},
        D3D12_INPUT_ELEMENT_DESC{"NORMAL",
                                 0,
                                 DXGI_FORMAT_R32G32B32_FLOAT,
                                 0,
                                 offsetof(VertexSpecification, normal),
                                 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                 0},
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.InputLayout = D3D12_INPUT_LAYOUT_DESC{inputElementDesc.data(),
                                                  inputElementDesc.size()};
    psoDesc.pRootSignature = rootSignature_.get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.get());
    CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
    // Show backfaces (e.g. faces inside Utah Teapot)
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState = rasterizerDesc;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc(D3D12_DEFAULT);
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    depthStencilDesc.StencilEnable = FALSE;
    psoDesc.DepthStencilState = depthStencilDesc;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = renderTargets_[0]->GetDesc().Format;
    psoDesc.DSVFormat = depthStencil_->GetDesc().Format;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    HRESULT hr = device_->CreateGraphicsPipelineState(&psoDesc,
                                                      IID_PPV_ARGS(pso_.put()));

    if (FAILED(hr))
    {
        utils::showErrorMessage("unable to create Pipeline State Object (PSO)");
        return false;
    }
    return true;
}

bool D3D12Renderer::compileShader(const fs::path& shaderPath,
                                  const ShaderCompileType shaderType,
                                  com_ptr<ID3DBlob>& shaderOut)
{
    com_ptr<ID3DBlob> error;
    constexpr UINT compileFlags =
#if defined(_DEBUG)
        // Skipping optimizations enable debugging with tools
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        0
#endif
    ;

    // UWP apps don't support D3DCompileFromFile() outside of development
    HRESULT hr = D3DCompileFromFile(
        shaderPath.c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main",
        shaderType == ShaderCompileType::VertexShader ? "vs_5_0" : "ps_5_0",
        compileFlags,
        0,
        shaderOut.put(),
        error.put());
    if (FAILED(hr))
    {
        std::string errorCause
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

void D3D12Renderer::createSyncObjects()
{
    device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.put()));
    fenceValue_ = 1;
    fenceEvent_ = ::CreateEvent(nullptr, false, false, nullptr);
}

bool D3D12Renderer::loadAssets()
{
    const std::array<fs::path, 3> modelPaths{"assets/meshes/cube.obj",
                                             "assets/meshes/teapot.obj",
                                             "assets/meshes/bunny.obj"};
    for (const auto& path : modelPaths)
    {
        std::optional<D3D12Model> model
            = D3D12Model::create(device_.get(), path);
        if (!model)
        {
            utils::showErrorMessage("unable to create model from path ", path);
            return false;
        }
        models_.emplace_back(std::move(model.value()));
    }

    return true;
}

void D3D12Renderer::initImGuiBackend()
{
    ImGui_ImplWin32_Init(window_.raw());

    // TODO: This is the legacy way of initializing a Direct3D12 ImGui backend.
    // From version 1.91.6, all ImGui backend versions require specifying a
    // separate ImGui_ImplDX12_InitInfo structure and the use of SRV descriptor
    // heap allocators.
    //
    // See:
    // https://github.com/ocornut/imgui/blob/c0dfd65d6790b9b96872b64fa232f1fa80fcd3b3/examples/example_win32_directx12/main.cpp#L37
    ImGui_ImplDX12_Init(
        device_.get(),
        FRAME_COUNT,
        renderTargets_[0]->GetDesc().Format,
        srvDescriptorHeap_.get(),
        srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
        srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart());
}

void D3D12Renderer::draw(const Scene& scene)
{
    // TODO: Update transformation calculations to reflect calculations in
    // GLRenderer
    MVPConstantBuffer mvpConstantBufferData;
    XMMATRIX projection
        = XMMatrixPerspectiveFovRH(XMConvertToRadians(drawProps_.fieldOfView),
                                   1024.0F / 768.0F,
                                   0.1F,
                                   100.0F);
    XMStoreFloat4x4(&mvpConstantBufferData.projection,
                    XMMatrixTranspose(projection));

    // TODO: Conversion between GLM and DXMath matrices is a performance
    // overhead
    const glm::mat4 glmView = camera_.calculateViewMatrix();
    // clang-format off
    XMMATRIX dxView = XMMatrixSet(
        glmView[0][0], glmView[1][0], glmView[2][0], glmView[3][0],
        glmView[0][1], glmView[1][1], glmView[2][1], glmView[3][1],
        glmView[0][2], glmView[1][2], glmView[2][2], glmView[3][2],
        glmView[0][3], glmView[1][3], glmView[2][3], glmView[3][3]
    );
    // clang-format on
    XMStoreFloat4x4(&mvpConstantBufferData.view, dxView);

    MaterialConstantBuffer materialConstantBufferData;
    materialConstantBufferData.viewPos.x = camera_.position().x;
    materialConstantBufferData.viewPos.y = camera_.position().y;
    materialConstantBufferData.viewPos.z = camera_.position().z;
    materialConstantBufferData.lightDirection.x = drawProps_.lightDirection[0];
    materialConstantBufferData.lightDirection.y = drawProps_.lightDirection[1];
    materialConstantBufferData.lightDirection.z = drawProps_.lightDirection[2];
    materialConstantBufferData.adsPropertiesFlags = 0;
    if (drawProps_.diffuseEnabled)
    {
        materialConstantBufferData.adsPropertiesFlags
            |= ADS_FLAG_DIFFUSE_ENABLED;
    }
    if (drawProps_.specularEnabled)
    {
        materialConstantBufferData.adsPropertiesFlags
            |= ADS_FLAG_SPECULAR_ENABLED;
    }

    // Begin frame (populate command list)
    commandAllocator_->Reset();
    commandList_->Reset(commandAllocator_.get(), pso_.get());

    // Set render state
    commandList_->SetGraphicsRootSignature(rootSignature_.get());

    // CSV
    const std::array<ID3D12DescriptorHeap*, 1> cbvHeaps
        = {cbvDescriptorHeap_.get()};
    commandList_->SetDescriptorHeaps(cbvHeaps.size(), cbvHeaps.data());

    // SRV
    const std::array<ID3D12DescriptorHeap*, 1> srvHeaps
        = {srvDescriptorHeap_.get()};
    commandList_->SetDescriptorHeaps(srvHeaps.size(), srvHeaps.data());

    // Set Rasterizer Stage state
    commandList_->RSSetViewports(1, &viewPort_);
    commandList_->RSSetScissorRects(1, &scissorRect_);

    // Set backbuffer as render target
    ID3D12Resource* renderTarget = renderTargets_[frameIndex_].get();
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        renderTarget,
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList_->ResourceBarrier(1, &barrier);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
        rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
        static_cast<int>(frameIndex_),
        rtvDescriptorHeapSize_);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(
        dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());
    commandList_->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

    // Record drawing commands
    const std::array<float, 4> clearColor = {drawProps_.backgroundColor[0],
                                             drawProps_.backgroundColor[1],
                                             drawProps_.backgroundColor[2],
                                             1.0F};
    commandList_->ClearRenderTargetView(rtvHandle,
                                        clearColor.data(),
                                        0,
                                        nullptr);
    commandList_->ClearDepthStencilView(dsvHandle,
                                        D3D12_CLEAR_FLAG_DEPTH,
                                        1.0F,
                                        0,
                                        0,
                                        nullptr);

    drawModels(scene, mvpConstantBufferData, materialConstantBufferData);

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList_.get());

    // End frame
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        renderTarget,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);
    commandList_->ResourceBarrier(1, &barrier);
    commandList_->Close();

    const std::array<ID3D12CommandList*, 1> commandLists = {commandList_.get()};
    commandQueue_->ExecuteCommandLists(commandLists.size(),
                                       commandLists.data());

    swapChain_->Present(vsyncEnabled_, 0);
    waitForPreviousFrame();
}

void D3D12Renderer::drawModels(
    const Scene& scene,
    MVPConstantBuffer& mvpConstantBufferData,
    MaterialConstantBuffer& materialConstantBufferData)
{
    for (size_t i = 0; i < scene.children().size(); ++i)
    {
        const SceneNode& sceneNode = scene.children()[i];
        const D3D12Model* model = &models_[sceneNode.modelID];

        // Model transform
        // Translate
        XMMATRIX modelMatrix = XMMatrixTranslation(sceneNode.position.x,
                                                   sceneNode.position.y,
                                                   sceneNode.position.z);

        // Avoid Gimbal-lock by converting Euler angles to quaternions
        const XMVECTOR quatX = XMQuaternionRotationAxis(
            XMVectorSet(1.0F, 0.0F, 0.0F, 0.0F),
            XMConvertToRadians(sceneNode.rotation.x));
        const XMVECTOR quatY = XMQuaternionRotationAxis(
            XMVectorSet(0.0F, 1.0F, 0.0F, 0.0F),
            XMConvertToRadians(sceneNode.rotation.y));
        const XMVECTOR quatZ = XMQuaternionRotationAxis(
            XMVectorSet(0.0F, 0.0F, 1.0F, 0.0F),
            XMConvertToRadians(sceneNode.rotation.z));
        const XMVECTOR quat
            = XMQuaternionMultiply(XMQuaternionMultiply(quatX, quatY), quatZ);
        const XMMATRIX rotation = XMMatrixRotationQuaternion(quat);

        modelMatrix = XMMatrixMultiply(rotation, modelMatrix);
        XMStoreFloat4x4(&mvpConstantBufferData.model,
                        XMMatrixTranspose(modelMatrix));

        // No need to transpose, as the inverse matrix is already in the correct
        // transposed state
        XMMATRIX normalMatrix = XMMatrixInverse(nullptr, modelMatrix);
        XMStoreFloat4x4(&mvpConstantBufferData.normalMatrix, normalMatrix);

        materialConstantBufferData.color.x = sceneNode.color.r;
        materialConstantBufferData.color.y = sceneNode.color.g;
        materialConstantBufferData.color.z = sceneNode.color.b;

        // Write MVP constant buffer
        std::memcpy(mvpCbvDataBegin_[i],
                    &mvpConstantBufferData,
                    sizeof(mvpConstantBufferData));
        CD3DX12_GPU_DESCRIPTOR_HANDLE mpvCbvHandle(
            cbvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart(),
            static_cast<int>(i),
            cbvDescriptorHeapSize_);
        commandList_->SetGraphicsRootDescriptorTable(0, mpvCbvHandle);

        // Write Material constant buffer
        std::memcpy(materialCbvDataBegin_[i],
                    &materialConstantBufferData,
                    sizeof(materialConstantBufferData));
        CD3DX12_GPU_DESCRIPTOR_HANDLE materialCbvHandle(
            cbvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart(),
            static_cast<int>(mvpConstantBuffers_.size() + i),
            cbvDescriptorHeapSize_);
        commandList_->SetGraphicsRootDescriptorTable(1, materialCbvHandle);

        // Issue draw call
        // TODO: Wireframe fill mode requires creation of a separate PSO with
        // rasterizer fill mode set to D3D12_FILL_MODE_WIREFRAME. Do not use
        // D3D_PRIMITIVE_TOPOLOGY_LINELIST, because it will have missing edges.
        commandList_->IASetPrimitiveTopology(
            D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList_->IASetVertexBuffers(0, 1, &model->vertexBuffer());
        commandList_->IASetIndexBuffer(&model->indexBuffer());
        commandList_->DrawIndexedInstanced(model->indexCount(), 1, 0, 0, 0);
    }
}

void D3D12Renderer::waitForPreviousFrame()
{
    // Wait for previous frame to complete before continue
    // (NOT BEST PRACTICE!)
    const UINT64 fence = fenceValue_;
    commandQueue_->Signal(fence_.get(), fence);
    ++fenceValue_;

    if (fence_->GetCompletedValue() < fence)
    {
        fence_->SetEventOnCompletion(fence, fenceEvent_);
        WaitForSingleObject(fenceEvent_, INFINITE);
    }
    frameIndex_ = swapChain_->GetCurrentBackBufferIndex();
}

