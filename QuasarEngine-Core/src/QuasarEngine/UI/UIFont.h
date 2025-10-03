#pragma once

#include <unordered_map>
#include <memory>
#include <vector>
#include <string>

namespace QuasarEngine
{
	class Texture2D;

	struct UIFontGlyph {
		float advance;
		float offsetX, offsetY;
		float w, h;
		float u0, v0, u1, v1;
	};

	class UIFont {
	public:
		UIFont();
		~UIFont() = default;

		bool LoadTTF(const std::string& ttfPath, float pixelHeight, int atlasW = 512, int atlasH = 512);
		std::string GetTextureId() const { return m_Id; }
		float Ascent() const { return m_Ascent; }
		float Descent() const { return m_Descent; }
		float LineGap() const { return m_LineGap; }
		const UIFontGlyph* GetGlyph(unsigned int codepoint) const;
	private:
		std::string m_Id;

		int m_AtlasW = 0, m_AtlasH = 0;
		float m_Ascent = 0, m_Descent = 0, m_LineGap = 0, m_Scale = 1.f;

		std::unordered_map<unsigned int, UIFontGlyph> m_Glyphs;
	};
}