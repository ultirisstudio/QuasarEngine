#pragma once

#include <QuasarEngine.h>
#include <QuasarEngine/Events/Event.h>
#include <QuasarEngine/Events/KeyEvent.h>
#include <QuasarEngine/Events/MouseEvent.h>

#include <QuasarEngine/Resources/ScreenQuad.h>
#include <QuasarEngine/Renderer/Framebuffer.h>
#include <QuasarEngine/Scene/Scene.h>
#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Shader/Shader.h>

#include "Gameplay/Player.h"
#include "World/Chunks/ChunkManager.h"

namespace QuasarEngine
{
	class Runtime : public Layer
	{
	public:
		Runtime();
		virtual ~Runtime() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(double dt) override;
		void OnRender() override;
		void OnGuiRender() override;
		void OnEvent(Event& e) override;

	private:
		std::unique_ptr<ScreenQuad> m_ScreenQuad;
		std::unique_ptr<Scene> m_Scene;
		std::shared_ptr<Framebuffer> m_FrameBuffer;
		std::shared_ptr<Shader> m_ScreenQuadShader;
		glm::vec2 m_ApplicationSize = { 0.0f, 0.0f };

	private:
		std::unique_ptr<Player> m_Player;
		std::unique_ptr<ChunkManager> m_ChunkManager;
	};
}