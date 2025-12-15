#ifndef D3D12_RENDERER_HPP_
#define D3D12_RENDERER_HPP_

#include "d3d12_model.hpp"
#include "d3d12_skybox.hpp"
#include "renderer.hpp"
#include "utils.hpp"
#include "win32_window.hpp"

#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <winrt/base.h>

class Scene;

class D3D12Renderer : public Renderer
{
public:
    D3D12Renderer(Window& window,
                  const DrawProperties& drawProps,
                  const Camera& camera);
    ~D3D12Renderer() override;
    DISABLE_COPY_AND_MOVE(D3D12Renderer)

    bool init() final;
    void initImGuiBackend() final;
    void cleanup() final;
    void draw(const Scene& scene) final;
    void setVSyncEnabled(const bool vsyncEnabled) final;

private:
    static constexpr size_t CONSTANT_BUFFER_ALIGNMENT = 256;
    struct alignas(CONSTANT_BUFFER_ALIGNMENT) MVPConstantBuffer
    {
        DirectX::XMFLOAT4X4 model;
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
        DirectX::XMFLOAT4X4 normalMatrix;
    };
    static_assert((sizeof(MVPConstantBuffer) % CONSTANT_BUFFER_ALIGNMENT) == 0,
                  "MVP Constant Buffer size must be 256-byte aligned");

    enum AdsPropertiesFlags : uint8_t
    {
        ADS_FLAG_DIFFUSE_ENABLED = 1,
        ADS_FLAG_SPECULAR_ENABLED = 2,
    };

    // See this link for HLSL Constant Buffer layout rules:
    // https://maraneshi.github.io/HLSL-ConstantBufferLayoutVisualizer/
    struct alignas(CONSTANT_BUFFER_ALIGNMENT) MaterialConstantBuffer
    {
        DirectX::XMFLOAT3 color;
        // Bit packing instead of bools to avoid wasting 4 byte for each bool
        uint32_t adsPropertiesFlags;
        DirectX::XMFLOAT3 viewPos;
        float padding;
        DirectX::XMFLOAT3 lightDirection;
    };
    static_assert((sizeof(MaterialConstantBuffer) % CONSTANT_BUFFER_ALIGNMENT)
                      == 0,
                  "Material Constant Buffer size must be 256-byte aligned");

    static constexpr UINT FRAME_COUNT = 2;
    static constexpr DXGI_FORMAT DEPTH_STENCIL_FORMAT
        = DXGI_FORMAT_D24_UNORM_S8_UINT;
    static constexpr size_t MAX_MODEL_SCENE_NODE_COUNT = 256;

    CD3DX12_VIEWPORT viewport_;
    CD3DX12_RECT scissorRect_;

    winrt::com_ptr<ID3D12Device> device_;
    winrt::com_ptr<IDXGISwapChain3> swapChain_;
    winrt::com_ptr<ID3D12CommandQueue> commandQueue_;
    winrt::com_ptr<ID3D12CommandAllocator> commandAllocator_;
    winrt::com_ptr<ID3D12GraphicsCommandList> commandList_;
    winrt::com_ptr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
    size_t rtvDescriptorHeapSize_;
    winrt::com_ptr<ID3D12DescriptorHeap> dsvDescriptorHeap_;

    static constexpr size_t NULL_CBV_COUNT = 1;
    static constexpr size_t CB_PER_MODEL_COUNT = 2;
    static constexpr size_t SKYBOX_TEXTURE_SRV_COUNT = 1;
    static constexpr size_t IMGUI_FONT_TEXTURE_SRV_COUNT = 1;
    static constexpr UINT MAX_CBV_SRV_UAV_DESCRIPTOR_COUNT
        = NULL_CBV_COUNT + (MAX_MODEL_SCENE_NODE_COUNT * CB_PER_MODEL_COUNT)
        + SKYBOX_TEXTURE_SRV_COUNT + IMGUI_FONT_TEXTURE_SRV_COUNT;
    winrt::com_ptr<ID3D12DescriptorHeap> cbvSrvUavDescriptorHeap_;
    size_t cbvSrvUavDescriptorHeapSize_;

    std::array<winrt::com_ptr<ID3D12Resource>, FRAME_COUNT> renderTargets_;
    winrt::com_ptr<ID3D12Resource> depthStencil_;
    // TODO: Move model PSO out of D3D12Renderer
    winrt::com_ptr<ID3D12RootSignature> modelRootSignature_;
    winrt::com_ptr<ID3D12PipelineState> modelPso_;
    std::vector<winrt::com_ptr<ID3D12Resource>> mvpConstantBuffers_;
    std::vector<winrt::com_ptr<ID3D12Resource>> materialConstantBuffers_;
    std::vector<uint8_t*> mvpCbvDataBegin_;
    std::vector<uint8_t*> materialCbvDataBegin_;

    winrt::com_ptr<ID3D12Fence> fence_;
    UINT frameIndex_;
    UINT64 fenceValue_;
    HANDLE fenceEvent_;

    bool vsyncEnabled_;
    std::vector<D3D12Model> models_;
    D3D12Skybox skybox_;

    bool createDevice();
    bool createCommandObjects();
    bool createDescriptorHeaps();
    bool createRTVs();
    bool createDSV();
    bool createCBVs();
    template <typename ConstantBufferType>
    bool createConstantBufferAndView(
        const CD3DX12_HEAP_PROPERTIES& heapProperties,
        winrt::com_ptr<ID3D12Resource>& constantBuffer,
        UINT cbvDescriptorHeapIndex,
        uint8_t*& cbvDataBegin);
    bool createModelRootSignature();
    bool createModelPSO();
    bool createSyncObjects();
    bool loadAssets();

    void drawModels(const Scene& scene,
                    MVPConstantBuffer& mvpConstantBufferData,
                    MaterialConstantBuffer& materialConstantBufferData);
    void drawGui();
    void screenshot();
    void waitForPreviousFrame();
};

inline void D3D12Renderer::setVSyncEnabled(const bool vsyncEnabled)
{
    vsyncEnabled_ = vsyncEnabled;
}

#endif
