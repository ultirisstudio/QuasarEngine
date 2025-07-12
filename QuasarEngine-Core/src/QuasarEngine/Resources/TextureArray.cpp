#include "qepch.h"
#include "TextureArray.h"

namespace QuasarEngine
{
	TextureArray::TextureArray(const TextureSpecification& specification) : Texture(specification)
	{
		
	}

	std::shared_ptr<TextureArray> TextureArray::CreateTextureArray(const TextureSpecification& specification)
	{
		return std::make_shared<TextureArray>(specification);
	}
}