#pragma once

#include "QuasarEngine/ImGui/ImGuiLayer.h"

struct GLFWwindow;
struct ID3D11Device;
struct ID3D11DeviceContext;

namespace QuasarEngine
{
    class DirectXImGuiLayer : public ImGuiLayer
    {
    public:
        DirectXImGuiLayer();
        ~DirectXImGuiLayer() override;

        void OnAttach() override;
        void OnDetach() override;

        void Begin() override;
        void End() override;

    private:
        GLFWwindow* m_Window = nullptr;
        ID3D11Device* m_Device = nullptr;
        ID3D11DeviceContext* m_DeviceContext = nullptr;
    };
}
