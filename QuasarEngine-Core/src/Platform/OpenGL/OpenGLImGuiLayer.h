#pragma once

#include "QuasarEngine/ImGui/ImGuiLayer.h"

namespace QuasarEngine
{
	class OpenGLImGuiLayer : public ImGuiLayer
	{
	public:
		OpenGLImGuiLayer();
		~OpenGLImGuiLayer() override;

		void OnAttach() override;
		void OnDetach() override;

		void Begin() override;
		void End() override;
	};
}