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

#include "imgui.h"
//#include "../bindings/imgui_impl_win32.h"
#include "../bindings/imgui_impl_glfw.h"
#include "../bindings/imgui_impl_dx11.h"


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

int bagacounter = 0;
int btncounter = 0;

nvrhi::TextureHandle swapChainTexture = NULL;
ID3D11Device* g_pd3dDevice = NULL;
ID3D11DeviceContext* g_pImmediateContext = NULL;
IDXGISwapChain* g_pSwapChain = NULL;
ID3D11RenderTargetView* g_pRenderTargetView = NULL;
nvrhi::DeviceHandle nvrhiDevice=NULL;
nvrhi::CommandListHandle commandList = NULL;
nvrhi::GraphicsPipelineHandle graphicsPipeline =NULL;
nvrhi::GraphicsPipelineHandle graphicsPipeline2 =NULL;
nvrhi::FramebufferHandle framebuffer=NULL;
nvrhi::InputLayoutHandle inputLayout = NULL;


nvrhi::ShaderHandle ptrvertexShader = nullptr;
nvrhi::ShaderHandle ptrpixelShader = nullptr;
nvrhi::ShaderHandle ptrvertexShader2 = nullptr;
nvrhi::ShaderHandle ptrpixelShader2 = nullptr;

std::string inputstr;

bool shaderchanged = false;
bool shaderchanged2 = false;

std::string ori_PixelShader = std::string("struct PSInput\n\
    {\n\
        float4 position: SV_POSITION;    \n\
        float3 color: COLOR;     \n\
    };     \n\
    struct PSOutput    \n\
    {     \n\
        float4 color: SV_TARGET;    \n\
    };    \n\
    PSOutput Main(PSInput input)      \n\
    {    \n\
        PSOutput output = (PSOutput)0;     \n\
        output.color.x = input.color.x * 0.1;    \n\
        output.color.y = input.color.y * 0.9;    \n\
        output.color.z = input.color.z;    \n\
        output.color.w = 1.0f;   \n\
        return output;    \n\
    }                                                                                             \0");


std::string tmp_PixelShader = ori_PixelShader;
std::string used_PixelShader = ori_PixelShader;
std::string tmp_PixelShader2 = ori_PixelShader;
std::string used_PixelShader2 = ori_PixelShader;

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


HRESULT CreateShaderFromStrint(nvrhi::ShaderHandle& ptrvertexShader,nvrhi::ShaderHandle& ptrpixelShader, std::string g_PixelShader)
{
    HRESULT hr = S_OK;
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
    // auto g_PixelShader = std::string("struct PSInput\
    // {\
    //     float4 position: SV_POSITION;\
    //     float3 color: COLOR;\
    // };\
    // struct PSOutput\
    // {\
    //     float4 color: SV_TARGET;\
    // };\
    // PSOutput Main(PSInput input)\
    // {\
    //     PSOutput output = (PSOutput)0;\
    //     output.color = float4(input.color, 1.0);\
    //     return output;\
    // }\0");

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
     ptrvertexShader = nvrhiDevice->createShader(
    vsshaderDesc,
        vshaderBlob->GetBufferPointer(), vshaderBlob->GetBufferSize());

    nvrhi::ShaderDesc psshaderDesc;
        psshaderDesc.shaderType = nvrhi::ShaderType::Pixel;
        psshaderDesc.debugName = "pixel";
        psshaderDesc.entryName = "Main";
    ptrpixelShader = nvrhiDevice->createShader(
    psshaderDesc,
    pshaderBlob->GetBufferPointer(), pshaderBlob->GetBufferSize());

    return hr;
}

HRESULT InitD3D(HWND OutputWindow, GLFWwindow *window)
{
    int sstrsize = ori_PixelShader.size()*2;
    ori_PixelShader.resize(sstrsize);

    bagacounter = 0;

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
    swapchainDescription.BufferDesc.Width = 800;
    swapchainDescription.BufferDesc.Height = 600;
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

    ShowWindow(OutputWindow, SW_HIDE);     
    UpdateWindow(OutputWindow);

    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.DisplaySize = ImVec2(960, 600);
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    //ImGui_ImplWin32_Init(OutputWindow);
    ImGui_ImplGlfw_InitForOther(window, true);
    
    //ImGui_ImplDX11_Init(g_pd3dDevice, g_pImmediateContext);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pImmediateContext);
    ImGui::StyleColorsDark();

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
    hr = CreateShaderFromStrint(ptrvertexShader, ptrpixelShader, g_PixelShader);

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

    inputLayout = nvrhiDevice->createInputLayout(
        attributes, uint32_t(std::size(attributes)), ptrvertexShader);
        
   
       
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
    graphicsPipeline2 = nvrhiDevice->createGraphicsPipeline(pipelineDesc, framebuffer);

    return hr;
}

auto callback_resize(ImGuiInputTextCallbackData* data) -> int
{
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
    {
        std::vector<char>* buf = (std::vector<char>*)data->UserData;
        buf->resize(data->BufTextLen + 1);  // +1 留给 '\0'
        data->Buf = buf->data();
    }
    return 0;
}

void Render()
{
    
    int tmp = (bagacounter / 100 + 1)%10;
    int tmpinv = 10 - tmp;
    //used_PixelShader.replace(318, 1, std::to_string(tmp));
    //used_PixelShader.replace(367, 1, std::to_string(tmpinv));
    bagacounter++;
    if (shaderchanged)
    {
        HRESULT hrr = CreateShaderFromStrint(ptrvertexShader, ptrpixelShader, used_PixelShader);
        shaderchanged = false;
   
    
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
    // float clearColor[4] = {0.0f, 0.0f, 0.7f, 1.0f};
    // g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);
    }
    if (shaderchanged2)
    {
        HRESULT hrr = CreateShaderFromStrint(ptrvertexShader2, ptrpixelShader2, used_PixelShader2);
        shaderchanged2 = false;
   
    
    auto pipelineDesc = nvrhi::GraphicsPipelineDesc()
    .setPrimType(nvrhi::PrimitiveType::TriangleList)
    .setInputLayout(inputLayout)
    .setVertexShader(ptrvertexShader2)
    .setPixelShader(ptrpixelShader2);
    //.addBindingLayout(bindingLayout);
    pipelineDesc.renderState.depthStencilState.depthTestEnable = false;
    pipelineDesc.renderState.depthStencilState.depthWriteEnable = false;
    pipelineDesc.renderState.depthStencilState.stencilEnable = false;
    pipelineDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
    

    graphicsPipeline2 = nvrhiDevice->createGraphicsPipeline(pipelineDesc, framebuffer);
    // float clearColor[4] = {0.0f, 0.0f, 0.7f, 1.0f};
    // g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);
    }
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
        .setViewport(nvrhi::ViewportState().addViewportAndScissorRect(nvrhi::Viewport(320.0f,640.f, 0.0f,240.f,0.0f,1.0f)))
        //.addBindingSet(bindingSet)
        .addVertexBuffer({vertexBuffer,0,0});

     // Clear the primary render target
    auto graphicsState2 = nvrhi::GraphicsState()
        .setPipeline(graphicsPipeline2)
        .setFramebuffer(framebuffer)
        .setViewport(nvrhi::ViewportState().addViewportAndScissorRect(nvrhi::Viewport(321.f,640.f, 241.f,480.f,0.0f,1.0f)))
        //.addBindingSet(bindingSet)
        .addVertexBuffer({vertexBuffer,0,0});

    commandList->setGraphicsState(graphicsState);

     ImGui_ImplDX11_NewFrame();
     //ImGui_ImplWin32_NewFrame();
     ImGui_ImplGlfw_NewFrame();
     ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

   

    auto drawArguments = nvrhi::DrawArguments()
        .setVertexCount(3);

   //ImGui::Begin("The Triangle");
    commandList->draw(drawArguments);
    //commandList->close();
    //nvrhiDevice->executeCommandList(commandList);

    commandList->setGraphicsState(graphicsState2);

    drawArguments = nvrhi::DrawArguments()
        .setVertexCount(3);

    commandList->draw(drawArguments);
    commandList->close();
    nvrhiDevice->executeCommandList(commandList);

    //ImGui::End();
     //ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);
    bool show_another_window = true;
    ImGui::Begin("PS shader Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    ImGui::InputTextMultiline("Hello", tmp_PixelShader.data(), tmp_PixelShader.size(), ImVec2(600, 300), ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackResize,
    callback_resize,
    &tmp_PixelShader);
    if (ImGui::Button("Update Shader"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
    {    
        used_PixelShader = tmp_PixelShader;
        shaderchanged = true;
    }
    ImGui::End();
    bool show_window = true;
    ImGui::Begin("PS shader Window2", &show_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    ImGui::InputTextMultiline("Hello", tmp_PixelShader2.data(), tmp_PixelShader2.size(), ImVec2(600, 300), ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackResize,
    callback_resize,
    &tmp_PixelShader2);
    if (ImGui::Button("Update Shader"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
    {    
        used_PixelShader2 = tmp_PixelShader2;
        shaderchanged2 = true;
    }
    ImGui::End();

    ImGui::Render();


    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    

    // commandList->close();
    // ImGui::Begin("The Triangle");
    // nvrhiDevice->executeCommandList(commandList);
    // ImGui::End();
    //ImGui::Render();
    
    g_pSwapChain->Present(0,0);

    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
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
    glfwMakeContextCurrent(window);
    if (!window) {
        std::cerr << "Failed to create GLFW window!\n";
        glfwTerminate();
        return -1;
    }

    InitD3D(glfwGetWin32Window(window), window);
    glfwShowWindow(window);
    
    while(!glfwWindowShouldClose(window)) {
       
        Render();
        glfwPollEvents();


        
    }
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    //ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
