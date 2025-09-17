#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iostream>

#include <glm/glm.hpp>

namespace QuasarEngine {

	enum class ShaderDataType
	{
		None = 0, Float, Vec2, Vec3, Vec4, Mat3, Mat4, Int, IVec2, IVec3, IVec4, Bool
	};

	static uint32_t ShaderDataTypeSize(ShaderDataType type)
	{
		switch (type)
		{
		case ShaderDataType::Float:    return 4;
		case ShaderDataType::Vec2:   return 4 * 2;
		case ShaderDataType::Vec3:   return 4 * 3;
		case ShaderDataType::Vec4:   return 4 * 4;
		case ShaderDataType::Mat3:     return 4 * 3 * 3;
		case ShaderDataType::Mat4:     return 4 * 4 * 4;
		case ShaderDataType::Int:      return 4;
		case ShaderDataType::IVec2:     return 4 * 2;
		case ShaderDataType::IVec3:     return 4 * 3;
		case ShaderDataType::IVec4:     return 4 * 4;
		case ShaderDataType::Bool:     return 1;
		}

		return 0;
	}

	struct BufferElement
	{
		std::string Name;
		ShaderDataType Type;
		uint32_t Size;
		size_t Offset;
		bool Normalized;

		BufferElement() = default;

		BufferElement(ShaderDataType type, const std::string& name, bool normalized = false)
			: Name(name), Type(type), Size(ShaderDataTypeSize(type)), Offset(0), Normalized(normalized)
		{
		}

		uint32_t GetComponentCount() const
		{
			switch (Type)
			{
			case ShaderDataType::Float:  return 1;
			case ShaderDataType::Vec2:   return 2;
			case ShaderDataType::Vec3:   return 3;
			case ShaderDataType::Vec4:   return 4;
			case ShaderDataType::Mat3:   return 3;
			case ShaderDataType::Mat4:   return 4;
			case ShaderDataType::Int:    return 1;
			case ShaderDataType::IVec2:  return 2;
			case ShaderDataType::IVec3:  return 3;
			case ShaderDataType::IVec4:  return 4;
			case ShaderDataType::Bool:   return 1;
			}

			return 0;
		}
	};

	class BufferLayout
	{
	public:
		BufferLayout() {}

		BufferLayout(std::initializer_list<BufferElement> elements)
			: m_Elements(elements)
		{
			CalculateOffsetsAndStride();
		}

		uint32_t GetStride() const { return m_Stride; }
		const std::vector<BufferElement>& GetElements() const { return m_Elements; }

		std::vector<BufferElement>::iterator begin() { return m_Elements.begin(); }
		std::vector<BufferElement>::iterator end() { return m_Elements.end(); }
		std::vector<BufferElement>::const_iterator begin() const { return m_Elements.begin(); }
		std::vector<BufferElement>::const_iterator end() const { return m_Elements.end(); }
	private:
		void CalculateOffsetsAndStride()
		{
			size_t offset = 0;
			m_Stride = 0;
			for (auto& element : m_Elements)
			{
				element.Offset = offset;
				offset += element.Size;
				m_Stride += element.Size;
			}
		}
	private:
		std::vector<BufferElement> m_Elements;
		uint32_t m_Stride = 0;
	};

	class VertexBuffer
	{
	public:
		virtual ~VertexBuffer() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void Upload(const void* data, uint32_t size) = 0;

		virtual void Reserve(uint32_t size) = 0;

		virtual size_t GetSize() const = 0;

		virtual const BufferLayout& GetLayout() const = 0;
		virtual void SetLayout(const BufferLayout& layout) = 0;

		static std::shared_ptr<VertexBuffer> Create();
		static std::shared_ptr<VertexBuffer> Create(uint32_t size);
		static std::shared_ptr<VertexBuffer> Create(const void* data, uint32_t size);
	};

	class IndexBuffer
	{
	public:
		virtual ~IndexBuffer() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void Upload(const void* data, uint32_t size) = 0;

		virtual void Reserve(uint32_t size) = 0;

		virtual size_t GetSize() const = 0;
		virtual uint32_t GetCount() const = 0;

		static std::shared_ptr<IndexBuffer> Create();
		static std::shared_ptr<IndexBuffer> Create(uint32_t size);
		static std::shared_ptr<IndexBuffer> Create(const void* data, uint32_t size);
	};
}