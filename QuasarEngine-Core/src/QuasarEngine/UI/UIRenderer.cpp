#include "qepch.h"

#include "UIRenderer.h"
#include "UITransform.h"
#include "UIStyle.h"

namespace QuasarEngine {
    void UIBatcher::Clear() {
        m_Vertices.clear(); m_Indices.clear(); m_Cmds.clear();
    }

    void UIBatcher::PushRect(const Rect& r, uint32_t rgba, const UIScissor* sc) {
        int v0 = (int)m_Vertices.size();
        m_Vertices.push_back({ r.x,       r.y,       0.f,0.f, rgba });
        m_Vertices.push_back({ r.x + r.w,   r.y,       0.f,0.f, rgba });
        m_Vertices.push_back({ r.x + r.w,   r.y + r.h,   0.f,0.f, rgba });
        m_Vertices.push_back({ r.x,       r.y + r.h,   0.f,0.f, rgba });
        int i0 = (int)m_Indices.size();
        m_Indices.push_back(v0 + 0); m_Indices.push_back(v0 + 1); m_Indices.push_back(v0 + 2);
        m_Indices.push_back(v0 + 0); m_Indices.push_back(v0 + 2); m_Indices.push_back(v0 + 3);
        UIDrawCmd cmd; cmd.vtxOffset = 0; cmd.idxOffset = i0; cmd.idxCount = 6;
        if (sc) cmd.scissor = *sc;
        m_Cmds.push_back(cmd);
    }

    void UIRenderContext::DrawDebugText(const char* s, float x, float y, const UIColor& color) {
        Rect r{ x, y, 8.f, 18.f };
        uint32_t rgba = PackRGBA8(color);
        for (const char* p = s; *p; ++p) {
            Rect bar{ r.x, r.y + r.h - 3.f, r.w, 2.f };
            batcher->PushRect(bar, rgba, nullptr);
            r.x += 8.f;
        }
    }

    void UIRenderer::Begin(int fbW, int fbH) {
        fbW_ = fbW; fbH_ = fbH;
        m_Batcher.Clear();
        m_Context.batcher = &m_Batcher;
    }

    void UIRenderer::End() {
        FlushToEngine();
    }

    void UIRenderer::FlushToEngine() {
        
    }
}