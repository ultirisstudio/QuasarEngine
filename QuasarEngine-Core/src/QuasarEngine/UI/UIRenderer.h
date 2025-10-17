#pragma once

#include <vector>
#include <cstdint>

#include "UITransform.h"
#include "UIStyle.h"
#include "UIFont.h"

#include <QuasarEngine/Shader/Shader.h>

#include <QuasarEngine/Renderer/Buffer.h>
#include <QuasarEngine/Renderer/VertexArray.h>
#include <QuasarEngine/Resources/Materials/Material.h>

namespace QuasarEngine {
	struct UIVertex {
		float x, y, u, v;
		uint32_t rgba;
	};

	struct UITexture {
		std::string id = "";
	};

	struct UIScissor {
		int x = 0, y = 0, w = 0, h = 0;
	};

	class UIBatcher;

	struct UIRenderContext {
		class UIBatcher* batcher = nullptr;
		UITexture whiteTex;
		float dpiScale = 1.f;

		UIFont* defaultFont = nullptr;

		void DrawText(const char* s, float x, float y, const UIColor& color);
	};

	struct UIDrawCmd {
		UITexture tex;
		UIScissor scissor;

		int vtxOffset = 0;
		int idxOffset = 0;
		int idxCount = 0;
	};

	class UIBatcher {
	public:
		void Clear();
		void PushRect(const Rect& r, UITexture tex, uint32_t rgba, const UIScissor* sc);
		void PushQuadUV(float x, float y, float w, float h, float u0, float v0, float u1, float v1, UITexture tex, uint32_t rgba, const UIScissor* sc = nullptr);

		const std::vector<UIVertex>& Vertices() const { return m_Vertices; }
		const std::vector<uint32_t>& Indices() const { return m_Indices; }
		const std::vector<UIDrawCmd>& Commands() const { return m_Cmds; }
	private:
		std::vector<UIVertex> m_Vertices;
		std::vector<uint32_t> m_Indices;
		std::vector<UIDrawCmd> m_Cmds;
	};

	class UIRenderer {
	public:
		UIRenderer();
		~UIRenderer();

		void Begin(int fbW, int fbH);
		void End();

		UIRenderContext& Ctx() { return m_Context; }
		UIBatcher& Batcher() { return m_Batcher; }

		Shader* GetShader() const { return m_Shader.get(); }

		void FlushToEngine();

	private:
		int m_FbW = 0, m_FbH = 0;
		UIRenderContext m_Context{};
		UIBatcher m_Batcher{};

		std::shared_ptr<Shader> m_Shader;

		std::shared_ptr<Material> m_Material;

		std::shared_ptr<VertexArray> m_VertexArray;
		std::shared_ptr<VertexBuffer> m_VertexBuffer;

		std::unique_ptr<UIFont> m_DefaultFont;
	};
}