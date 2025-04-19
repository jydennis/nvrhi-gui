#include <nvrhi/nvrhi.h>
#include <nvrhi/d3d11.h>
#include <nvrhi/utils.h>
#include <GLFW/glfw3.h>
//#include <d3d11.h>
#include <d3dcompiler.h>
#include <C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Include/D3DX11.h>
#include <iostream>
#define GLFW_EXPOSE_NATIVE_WIN32 1
#include <GLFW/glfw3native.h>

struct Vertex {
    float position[3];
    float color[3];
};

static const Vertex g_Vertices[3]  {
    //  position          COLOR
    { { -0.5f, -0.5f, 0.f }, { 1.f, 0.f, 0.f } },
    { { 0.5f, -0.5f, 0.f }, { 0.f, 1.f, 0.f } },
    { { 0.0f, 0.5f, 0.f }, { 0.f, 0.f, 1.f } },
    // and so on...
};

nvrhi::TextureHandle swapChainTexture = NULL;
ID3D11Device* g_pd3dDevice = NULL;
ID3D11DeviceContext* g_pImmediateContext = NULL;
IDXGISwapChain* g_pSwapChain = NULL;
ID3D11RenderTargetView* g_pRenderTargetView = NULL;
nvrhi::DeviceHandle nvrhiDevice=NULL;
nvrhi::CommandListHandle commandList = NULL;
nvrhi::GraphicsPipelineHandle graphicsPipeline =NULL;
nvrhi::FramebufferHandle framebuffer=NULL;


struct MessageCallback : public nvrhi::IMessageCallback
{
	MessageCallback(uint8_t InMinLogLevel = 0)
		: MinLogLevel(InMinLogLevel)
	{}

	static nvrhi::IMessageCallback* GetDefault()
	{
		static MessageCallback DefaultCallback;
		return &DefaultCallback;
	}

	virtual void message(nvrhi::MessageSeverity Severity, const char* MessageText)
	{
		uint8_t LogLevel = uint8_t(Severity);
		const char* SeverityNames[4] = \
		{
			"Info",
			"Warning",
			"Error",
			"Fatal"
		};

		if (LogLevel >= MinLogLevel && MinLogLevel < 4)
		{
			const char* Severity = SeverityNames[LogLevel];
			std::cout << "NVRHI " << Severity << ": " << MessageText << "\n";
		}
	}

private:
	uint8_t MinLogLevel;
};


HRESULT InitD3D(HWND OutputWindow)
{
    HRESULT hr = S_OK;

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };

    DXGI_SWAP_CHAIN_DESC swapchainDescription;
    ZeroMemory(&swapchainDescription, sizeof(swapchainDescription));
    swapchainDescription.BufferCount = 1;
    swapchainDescription.BufferDesc.Width = 640;
    swapchainDescription.BufferDesc.Height = 480;
    swapchainDescription.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchainDescription.BufferDesc.RefreshRate.Numerator = 60;
    swapchainDescription.BufferDesc.RefreshRate.Denominator = 1;
    swapchainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchainDescription.OutputWindow = OutputWindow;
    swapchainDescription.SampleDesc.Count = 1;
    swapchainDescription.SampleDesc.Quality = 0;
    swapchainDescription.Windowed = TRUE;

    D3D_DRIVER_TYPE outDriverType;
    D3D_FEATURE_LEVEL outFeatureLevel;

    for (UINT driverTypeIndex = 0; driverTypeIndex < ARRAYSIZE(driverTypes); driverTypeIndex++)
    {
        outDriverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain(NULL, outDriverType, NULL, 0, featureLevels, ARRAYSIZE(featureLevels), \
            D3D11_SDK_VERSION, &swapchainDescription, &g_pSwapChain, &g_pd3dDevice, &outFeatureLevel, &g_pImmediateContext);
        
        if (SUCCEEDED(hr))
        {
            std::cerr << std::hex << outDriverType << " ";
            break;
        }
    }
    std::cerr << std::hex << outFeatureLevel;
    if(FAILED(hr))
    {
        return hr;
    }

    nvrhi::d3d11::DeviceDesc deviceDesc;
    deviceDesc.messageCallback =  MessageCallback::GetDefault();
    deviceDesc.context = g_pImmediateContext;

    nvrhiDevice = nvrhi::d3d11::createDevice(deviceDesc);

    nvrhi::RefCountPtr<ID3D11Texture2D> pBackBuffer = NULL;
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if(FAILED(hr))
    {
        return hr;
    }
    auto textureDesc = nvrhi::TextureDesc()
    .setDimension(nvrhi::TextureDimension::Texture2D)
    .setFormat(nvrhi::Format::RGBA8_UNORM)
    .setWidth(640)
    .setHeight(480)
    .setIsRenderTarget(true)
    .setClearValue(nvrhi::Color{ 0.0f, 0.0f, 0.7f, 1.0f })
    .setDebugName("Swap Chain Image")
    .setIsUAV(false);

    // In this line, <type> depends on the GAPI and should be one of: D3D11_Resource, D3D12_Resource, VK_Image.
    swapChainTexture = nvrhiDevice->createHandleForNativeTexture(nvrhi::ObjectTypes::D3D11_Resource, static_cast<ID3D11Resource*>(pBackBuffer.Get()), textureDesc);


    // hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
    // //pBackBuffer->Release();
    // if(FAILED(hr))
    // {
    //     return hr;
    // }

    // g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);


    // Assume the shaders are included as C headers; they could just as well be loaded from files.
    auto g_VertexShader = std::string("struct VSInput\
            {\
                float3 position: POSITION;\
                float3 color: COLOR;\
            };\
            struct VSOutput\
            {\
                float4 position: SV_POSITION;\
                float3 color: COLOR;\
            };\
            VSOutput Main(VSInput input)\
            {\
                VSOutput output = (VSOutput)0;\
                output.position = float4(input.position, 1.0);\
                output.color = input.color;\
                return output;\
            }\0");
    auto g_PixelShader = std::string("struct PSInput\
    {\
        float4 position: SV_POSITION;\
        float3 color: COLOR;\
    };\
    struct PSOutput\
    {\
        float4 color: SV_TARGET;\
    };\
    PSOutput Main(PSInput input)\
    {\
        PSOutput output = (PSOutput)0;\
        output.color = float4(input.color, 1.0);\
        return output;\
    }\0");

     const D3D_SHADER_MACRO defines[] = 
    {
        "EXAMPLE_DEFINE", "1",
        NULL, NULL
    };

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
    ID3DBlob* vshaderBlob = nullptr;
    ID3DBlob* pshaderBlob = nullptr;
    ID3DBlob* errorBlob1 = nullptr;
    ID3DBlob* errorBlob2 = nullptr;
    hr = D3DCompile( g_VertexShader.c_str(), g_VertexShader.length(), nullptr, nullptr, nullptr,
                                     "Main", "vs_5_0",
                                     flags, 0, &vshaderBlob, &errorBlob1 );
    if ( FAILED(hr) )
    {
        if ( errorBlob1 )
        {
            std::cerr << "here1";
            OutputDebugStringA( (char*)errorBlob1->GetBufferPointer() );
            errorBlob1->Release();
        }

        if ( vshaderBlob )
           vshaderBlob->Release();

        return hr;
    }    

    hr = D3DCompile( g_PixelShader.c_str(), g_PixelShader.length(), nullptr,nullptr, nullptr,
                                     "Main", "ps_5_0",
                                     flags, 0, &pshaderBlob, &errorBlob2 );
    
    if ( FAILED(hr) )
    {
        if ( errorBlob2 )
        {
            std::cerr << "here2";
            OutputDebugStringA( (char*)errorBlob2->GetBufferPointer() );
            errorBlob2->Release();
        }

        if ( pshaderBlob )
           pshaderBlob->Release();

        return hr;
    } 
    nvrhi::ShaderDesc vsshaderDesc;
        vsshaderDesc.shaderType = nvrhi::ShaderType::Vertex;
        vsshaderDesc.debugName = "vertex";
        vsshaderDesc.entryName = "Main";
    nvrhi::ShaderHandle ptrvertexShader = nvrhiDevice->createShader(
       vsshaderDesc,
        vshaderBlob->GetBufferPointer(), vshaderBlob->GetBufferSize());

    nvrhi::VertexAttributeDesc attributes[]  {
        nvrhi::VertexAttributeDesc()
            .setName("POSITION")
            .setFormat(nvrhi::Format::RGB32_FLOAT)
            .setOffset(0)//offsetof(Vertex, position))
            .setElementStride(24),//sizeof(Vertex)),
        nvrhi::VertexAttributeDesc()
            .setName("COLOR")
            .setFormat(nvrhi::Format::RGB32_FLOAT)
            .setOffset(12)//offsetof(Vertex, color))
            .setElementStride(24),//sizeof(Vertex)),
    };

    nvrhi::InputLayoutHandle inputLayout = nvrhiDevice->createInputLayout(
        attributes, uint32_t(std::size(attributes)), ptrvertexShader);
        
    nvrhi::ShaderDesc psshaderDesc;
        psshaderDesc.shaderType = nvrhi::ShaderType::Pixel;
        psshaderDesc.debugName = "pixel";
        psshaderDesc.entryName = "Main";
    nvrhi::ShaderHandle ptrpixelShader = nvrhiDevice->createShader(
       psshaderDesc,
       pshaderBlob->GetBufferPointer(), pshaderBlob->GetBufferSize());
       
    auto framebufferDesc = nvrhi::FramebufferDesc()
    .addColorAttachment(swapChainTexture); // you can specify a particular subresource if necessary

    framebuffer = nvrhiDevice->createFramebuffer(framebufferDesc);
    if (  framebuffer == nullptr){
        std::cerr<< "Failed to create Scene::framebuffer";
        return false;
    }
    
    
    auto pipelineDesc = nvrhi::GraphicsPipelineDesc()
    .setPrimType(nvrhi::PrimitiveType::TriangleList)
    .setInputLayout(inputLayout)
    .setVertexShader(ptrvertexShader)
    .setPixelShader(ptrpixelShader);
    //.addBindingLayout(bindingLayout);
    pipelineDesc.renderState.depthStencilState.depthTestEnable = false;
    pipelineDesc.renderState.depthStencilState.depthWriteEnable = false;
    pipelineDesc.renderState.depthStencilState.stencilEnable = false;
    pipelineDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;

    graphicsPipeline = nvrhiDevice->createGraphicsPipeline(pipelineDesc, framebuffer);

    return hr;
}

void Render()
{
    // float clearColor[4] = {0.0f, 0.0f, 0.7f, 1.0f};
    // g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);

    commandList= nvrhiDevice->createCommandList();

    commandList->open();

    commandList->clearTextureFloat(swapChainTexture, nvrhi::AllSubresources, nvrhi::Color{  0.0f, 0.0f, 0.7f, 1.0f });
    // Draw our geometry

    auto vertexBufferDesc = nvrhi::BufferDesc()
        .setByteSize(sizeof(g_Vertices))
        .setIsVertexBuffer(true)
        .setInitialState(nvrhi::ResourceStates::VertexBuffer)
        .setKeepInitialState(true) // enable fully automatic state tracking
        .setDebugName("Vertex Buffer");

    nvrhi::BufferHandle vertexBuffer  = nvrhiDevice->createBuffer(vertexBufferDesc);

    commandList->writeBuffer(vertexBuffer, g_Vertices, sizeof(g_Vertices));

    // auto bindingSetDesc = nvrhi::BindingSetDesc();

    // nvrhi::BindingSetHandle bindingSet = nvrhiDevice->createBindingSet(bindingSetDesc, bindingLayout);

    // Set the graphics state: pipeline, framebuffer, viewport, bindings.
    auto graphicsState = nvrhi::GraphicsState()
        .setPipeline(graphicsPipeline)
        .setFramebuffer(framebuffer)
        .setViewport(nvrhi::ViewportState().addViewportAndScissorRect(nvrhi::Viewport(640.f, 480.f)))
        //.addBindingSet(bindingSet)
        .addVertexBuffer({vertexBuffer,0,0});
    commandList->setGraphicsState(graphicsState);
     // Clear the primary render target
   
    auto drawArguments = nvrhi::DrawArguments()
        .setVertexCount(3);
    commandList->draw(drawArguments);
    

    commandList->close();
    nvrhiDevice->executeCommandList(commandList);

    g_pSwapChain->Present(0,0);
}

int main()
{
    // 1. 初始化 GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!\n";
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "NVRHI + DX11 Demo", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window!\n";
        glfwTerminate();
        return -1;
    }

    InitD3D(glfwGetWin32Window(window));
    glfwShowWindow(window);
    while(!glfwWindowShouldClose(window)) {
        Render();
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
