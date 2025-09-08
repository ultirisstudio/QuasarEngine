#include "qepch.h"
#include "DirectXContext.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <d3d11.h>
#include <dxgi.h>
#include <stdexcept>

#include <QuasarEngine/Core/Application.h>

namespace QuasarEngine
{
    DirectXContext::DirectXContextData DirectXContext::Context = DirectXContextData();

    DirectXContext::DirectXContext(GLFWwindow* windowHandle)
        : m_WindowHandle(windowHandle)
    {
        m_hWnd = glfwGetWin32Window(m_WindowHandle);
        Init();
    }

    DirectXContext::~DirectXContext()
    {
        Context.device = nullptr;
        Context.deviceContext = nullptr;
        Context.swapChain = nullptr;
        Context.renderTargetView = nullptr;
    }

    void DirectXContext::Init()
    {
        RECT rect;
        GetClientRect(m_hWnd, &rect);
        Context.width = rect.right - rect.left;
        Context.height = rect.bottom - rect.top;

        DXGI_SWAP_CHAIN_DESC scd = {};
        scd.BufferCount = 1;
        scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd.OutputWindow = m_hWnd;
        scd.SampleDesc.Count = 1;
        scd.Windowed = TRUE;

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            0,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &scd,
            Context.swapChain.GetAddressOf(),
            Context.device.GetAddressOf(),
            nullptr,
            Context.deviceContext.GetAddressOf()
        );

        if (FAILED(hr))
            throw std::runtime_error("Failed to create DirectX 11 device and swap chain");

        Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
        hr = Context.swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBuffer.GetAddressOf());
        if (FAILED(hr))
            throw std::runtime_error("Failed to get swap chain back buffer");

        hr = Context.device->CreateRenderTargetView(backBuffer.Get(), nullptr, Context.renderTargetView.GetAddressOf());
        if (FAILED(hr))
            throw std::runtime_error("Failed to create render target view");

        Context.deviceContext->OMSetRenderTargets(1, Context.renderTargetView.GetAddressOf(), nullptr);
    }

    void DirectXContext::BeginFrame()
    {
        float clearColor[4] = { 0.1f, 0.1f, 0.3f, 1.0f };
        Context.deviceContext->ClearRenderTargetView(Context.renderTargetView.Get(), clearColor);
    }

    void DirectXContext::EndFrame()
    {
        Window::WindowData& data = *(Window::WindowData*)glfwGetWindowUserPointer(reinterpret_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow()));
        Context.swapChain->Present(data.VSync ? 1 : 0, 0);
    }

    void DirectXContext::Resize(unsigned int newWidth, unsigned int newHeight)
    {
        Context.width = newWidth;
        Context.height = newHeight;

        Context.renderTargetView.Reset();

        HRESULT hr = Context.swapChain->ResizeBuffers(0, newWidth, newHeight, DXGI_FORMAT_UNKNOWN, 0);
        if (FAILED(hr))
            throw std::runtime_error("Failed to resize swap chain buffers");

        Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
        Context.swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBuffer.GetAddressOf());
        Context.device->CreateRenderTargetView(backBuffer.Get(), nullptr, Context.renderTargetView.GetAddressOf());

        Context.deviceContext->OMSetRenderTargets(1, Context.renderTargetView.GetAddressOf(), nullptr);
    }
}
