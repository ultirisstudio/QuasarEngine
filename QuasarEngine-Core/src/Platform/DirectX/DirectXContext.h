#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include "QuasarEngine/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace QuasarEngine {

    class DirectXContext : public GraphicsContext
    {
    public:
        DirectXContext(GLFWwindow* windowHandle);
        ~DirectXContext();

        DirectXContext(const DirectXContext&) = delete;
        DirectXContext& operator=(const DirectXContext&) = delete;

        void BeginFrame() override;
        void EndFrame() override;
        void Resize(unsigned int width, unsigned int height) override;

    private:
        void Init();

    private:
        GLFWwindow* m_WindowHandle;
        HWND m_hWnd = nullptr;

        struct DirectXContextData
        {
            Microsoft::WRL::ComPtr<ID3D11Device> device;
            Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext;
            Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
            Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;

            unsigned int width = 0;
            unsigned int height = 0;
        };
    public:

        static DirectXContextData Context;
    };
}
