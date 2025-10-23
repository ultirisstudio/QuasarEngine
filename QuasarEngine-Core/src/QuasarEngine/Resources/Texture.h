#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <QuasarEngine/Asset/Asset.h>
#include <QuasarEngine/Resources/TextureTypes.h>

namespace QuasarEngine
{
	using TextureHandle = std::uintptr_t;

	struct ByteView {
		const std::uint8_t* data{ nullptr };
		std::size_t size{ 0 };
		constexpr bool empty() const noexcept { return !data || size == 0; }
	};

	class Texture : public Asset
	{
	protected:
		TextureSpecification m_Specification{};

	public:
		explicit Texture(const TextureSpecification& specification);
		virtual ~Texture() = default;

		virtual TextureHandle GetHandle() const noexcept = 0;
		virtual bool IsLoaded() const noexcept = 0;

		const TextureSpecification& GetSpecification() const noexcept { return m_Specification; }

		virtual void Bind(int index = 0) const = 0;
		virtual void Unbind() const = 0;

		virtual void GenerateMips() = 0;

		virtual bool LoadFromPath(const std::string& path) = 0;
		virtual bool LoadFromMemory(ByteView data) = 0;
		virtual bool LoadFromData(ByteView data) = 0;

		static AssetType GetStaticType() { return AssetType::TEXTURE; }
		AssetType GetType() override { return GetStaticType(); }
	};
}