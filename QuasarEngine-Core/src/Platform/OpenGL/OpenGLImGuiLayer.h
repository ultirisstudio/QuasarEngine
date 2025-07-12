#pragma once

#include "QuasarEngine/ImGui/ImGuiLayer.h"

namespace QuasarEngine
{
	class OpenGLImGuiLayer : public ImGuiLayer
	{
	public:
		OpenGLImGuiLayer();
		~OpenGLImGuiLayer() override;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void Begin() override;
		virtual void End() override;
	};
}