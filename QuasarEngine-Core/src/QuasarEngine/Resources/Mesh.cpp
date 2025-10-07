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
		float maxX = -std::numeric_limits<float>::max();
		float maxY = -std::numeric_limits<float>::max();
		float maxZ = -std::numeric_limits<float>::max();

		const uint32_t strideFloats =
			static_cast<uint32_t>(m_vertexBuffer->GetLayout().GetStride() / sizeof(float));
		if (strideFloats < 3) return;

		for (size_t i = 0; i + 2 < vertices.size(); i += strideFloats)
		{
			minX = std::min(minX, vertices[i + 0]);
			minY = std::min(minY, vertices[i + 1]);
			minZ = std::min(minZ, vertices[i + 2]);

			maxX = std::max(maxX, vertices[i + 0]);
			maxY = std::max(maxY, vertices[i + 1]);
			maxZ = std::max(maxZ, vertices[i + 2]);
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
			RenderCommand::Instance().DrawArrays(m_drawMode, size);
		else
			RenderCommand::Instance().DrawElements(m_drawMode, count);
	}

	void Mesh::GenerateMesh(std::vector<float> vertices, std::vector<unsigned int> indices, std::optional<BufferLayout> layout)
	{
		m_vertexArray = VertexArray::Create();

		if (layout.has_value())
		{
			m_vertexBuffer = VertexBuffer::Create(vertices.data(), vertices.size() * sizeof(float));
			m_vertexBuffer->SetLayout(layout.value());
		}
		else
		{
			m_vertexBuffer = VertexBuffer::Create(vertices.data(), vertices.size() * sizeof(float));
			m_vertexBuffer->SetLayout({
				{ ShaderDataType::Vec3, "inPosition" },
				{ ShaderDataType::Vec3, "inNormal" },
				{ ShaderDataType::Vec2, "inTexCoord" }
			});
		}		
		m_vertexArray->AddVertexBuffer(m_vertexBuffer);

		std::shared_ptr<IndexBuffer> indexBuffer = IndexBuffer::Create(indices.data(), indices.size() * sizeof(unsigned int));
		m_vertexArray->SetIndexBuffer(indexBuffer);

		m_vertices = std::move(vertices);
		m_indices = std::move(indices);

		CalculateBoundingBoxSize(m_vertices);

		m_meshGenerated = true;
	}

	bool Mesh::IsVisible(const Math::Frustum& frustum, const glm::mat4& modelMatrix) const
	{
		const glm::vec3 bbMinLocal = m_boundingBoxPosition;
		const glm::vec3 bbSizeLocal = m_boundingBoxSize;
		const glm::vec3 bbCenterL = bbMinLocal + 0.5f * bbSizeLocal;
		const glm::vec3 bbHalfL = 0.5f * bbSizeLocal;

		const glm::vec3 centerW = glm::vec3(modelMatrix * glm::vec4(bbCenterL, 1.0f));

		const glm::vec3 axisX = glm::vec3(modelMatrix[0]) * bbHalfL.x;
		const glm::vec3 axisY = glm::vec3(modelMatrix[1]) * bbHalfL.y;
		const glm::vec3 axisZ = glm::vec3(modelMatrix[2]) * bbHalfL.z;

		for (int i = 0; i < 6; ++i)
		{
			const glm::vec3 n = frustum.planes[i].n;
			const float d = frustum.planes[i].d;

			const float dist = glm::dot(n, centerW) + d;
			
			const float r = std::abs(glm::dot(n, axisX))
				+ std::abs(glm::dot(n, axisY))
				+ std::abs(glm::dot(n, axisZ));

			if (dist + r < 0.0f)
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