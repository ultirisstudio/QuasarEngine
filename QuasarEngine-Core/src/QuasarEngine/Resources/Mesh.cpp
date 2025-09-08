#include "qepch.h"

#include <glm/gtx/matrix_decompose.hpp>

#include <QuasarEngine/Resources/Mesh.h>
#include <QuasarEngine/Renderer/RenderCommand.h>

namespace QuasarEngine
{
	void Mesh::CalculateBoundingBoxSize(std::vector<float> vertices)
	{
		float minX = std::numeric_limits<float>::max();
		float minY = std::numeric_limits<float>::max();
		float minZ = std::numeric_limits<float>::max();

		float maxX = std::numeric_limits<float>::min();
		float maxY = std::numeric_limits<float>::min();
		float maxZ = std::numeric_limits<float>::min();

		uint32_t stride = m_vertexBuffer->GetLayout().GetStride();

		for (int i = 0; i < vertices.size(); i += stride)
		{
			if (vertices[i + 0] < minX)
				minX = vertices[i + 0];
			if (vertices[i + 1] < minY)
				minY = vertices[i + 1];
			if (vertices[i + 2] < minZ)
				minZ = vertices[i + 2];

			if (vertices[i + 0] > maxX)
				maxX = vertices[i + 0];
			if (vertices[i + 1] > maxY)
				maxY = vertices[i + 1];
			if (vertices[i + 2] > maxZ)
				maxZ = vertices[i + 2];
		}

		m_boundingBoxSize = glm::vec3(maxX - minX, maxY - minY, maxZ - minZ);
		m_boundingBoxPosition = glm::vec3(minX, minY, minZ);
	}

	Mesh::Mesh(std::vector<float> vertices, std::vector<unsigned int> indices, std::optional<BufferLayout> layout, DrawMode drawMode, std::optional<MaterialSpecification> material) :
		m_drawMode(drawMode),
		m_vertices(vertices),
		m_indices(indices),
		m_material(material)
	{
		GenerateMesh(vertices, indices, layout);
	}

	Mesh::~Mesh()
	{
		m_vertexBuffer.reset();
		m_vertexArray.reset();
	}

	void Mesh::draw() const
	{
		m_vertexArray->Bind();

		uint32_t size = m_vertexBuffer->GetSize();

		std::shared_ptr<IndexBuffer> indexBuffer = m_vertexArray->GetIndexBuffer();
		uint32_t count = indexBuffer ? indexBuffer->GetCount() : 0;

		if (count == 0)
			RenderCommand::DrawArrays(m_drawMode, size);
		else
			RenderCommand::DrawElements(m_drawMode, count);
	}

	void Mesh::GenerateMesh(std::vector<float> vertices, std::vector<unsigned int> indices, std::optional<BufferLayout> layout)
	{
		m_vertexArray = VertexArray::Create();

		if (layout.has_value())
		{
			m_vertexBuffer = VertexBuffer::Create(vertices);
			m_vertexBuffer->SetLayout(layout.value());
		}
		else
		{
			m_vertexBuffer = VertexBuffer::Create(vertices);
			m_vertexBuffer->SetLayout({
				{ ShaderDataType::Vec3, "inPosition" },
				{ ShaderDataType::Vec3, "inNormal" },
				{ ShaderDataType::Vec2, "inTexCoord" }
			});
		}		
		m_vertexArray->AddVertexBuffer(m_vertexBuffer);

		std::shared_ptr<IndexBuffer> indexBuffer = IndexBuffer::Create(indices);
		m_vertexArray->SetIndexBuffer(indexBuffer);

		m_vertices = std::move(vertices);
		m_indices = std::move(indices);

		CalculateBoundingBoxSize(m_vertices);

		m_meshGenerated = true;
	}

	bool Mesh::IsVisible(const Math::Frustum& frustum, const glm::mat4& modelMatrix) const
	{
		glm::vec3 position;
		glm::decompose(modelMatrix, glm::vec3(), glm::quat(), position, glm::vec3(), glm::vec4());
		for (unsigned i = 0; i < std::size(frustum.planes); i++)
		{
			glm::vec3 positive = position + m_boundingBoxPosition;
			if (frustum.planes[i].a >= 0)
				positive.x += m_boundingBoxSize.x;
			if (frustum.planes[i].b >= 0)
				positive.y += m_boundingBoxSize.y;
			if (frustum.planes[i].c >= 0)
				positive.z += m_boundingBoxSize.z;

			if (positive.x * frustum.planes[i].a + positive.y * frustum.planes[i].b + positive.z * frustum.planes[i].c + frustum.planes[i].d < 0)
				return false;
		}

		return true;
	}

	bool Mesh::IsMeshGenerated()
	{
		return m_meshGenerated;
	}

	void Mesh::Clear()
	{
		m_vertices.clear();
		m_indices.clear();
	}
}