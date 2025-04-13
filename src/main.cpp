//#include <nvrhi/nvrhi.h>
//#include <nvrhi/d3d11.h>
#include <GLFW/glfw3.h>
#include <d3d11.h>
#include <D3DX11.h>
#include <iostream>
#define GLFW_EXPOSE_NATIVE_WIN32 1
#include <GLFW/glfw3native.h>

ID3D11Device* g_pd3dDevice = NULL;
ID3D11DeviceContext* g_pImmediateContext = NULL;
IDXGISwapChain* g_pSwapChain = NULL;
ID3D11RenderTargetView* g_pRenderTargetView = NULL;

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
            break;
        }
    }

    if(FAILED(hr))
    {
        return hr;
    }

    ID3D11Texture2D* pBackBuffer = NULL;
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if(FAILED(hr))
    {
        return hr;
    }

    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
    pBackBuffer->Release();
    if(FAILED(hr))
    {
        return hr;
    }

    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);

    return hr;
}

void Render()
{
    float clearColor[4] = {0.0f, 0.0f, 0.7f, 1.0f};
    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);

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

    while(!glfwWindowShouldClose(window)) {
        Render();

        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
