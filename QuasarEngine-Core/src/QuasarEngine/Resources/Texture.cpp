#include "qepch.h"
#include "Texture.h"

#include <fstream>

#include <QuasarEngine/Renderer/Renderer.h>

namespace QuasarEngine
{
	static uint32_t TEXTURE_COUNT = 0;

	Texture::Texture(const TextureSpecification& specification) : m_ID(TEXTURE_COUNT++), m_Specification(specification)
	{
		
	}
}