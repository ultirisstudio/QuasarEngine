#pragma once

#include "QuasarEngine/ImGui/ImGuiLayer.h"

#include <vulkan/vulkan.h>

namespace QuasarEngine
{
	class VulkanImGuiLayer : public ImGuiLayer, public Layer
	{
	public:
		VulkanImGuiLayer();
		~VulkanImGuiLayer() override;

		void OnAttach() override;
		void OnDetach() override;

		void Begin() override;
		void End() override;

	private:
		VkDescriptorPool descriptorPool;
	};
}
