#pragma once

#include <vector>
#include <string>

namespace QuasarEngine
{
	class CubeMap
	{
	public:
		CubeMap();

		void Load(std::array<std::string, 6> paths);
		void Load(std::string hdr_path);
		void ActiveTexture();
		void BeginDrawModel();
		void EndDrawModel();
		void Bind();

		unsigned int GetID() { return m_ID; }
	private:
		unsigned int m_ID;
	};
}