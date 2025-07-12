#pragma once

#include "QuasarEngine/Core/Layer.h"

namespace QuasarEngine
{
	class ImGuiLayer : public Layer
	{
	public:
		explicit ImGuiLayer(const std::string& name);
		virtual ~ImGuiLayer() = default;

		virtual void OnAttach() = 0;
		virtual void OnDetach() = 0;

		virtual void Begin() = 0;
		virtual void End() = 0;

		static std::unique_ptr<ImGuiLayer> Create();
	};
}