#pragma once

#include <vector>

#include <QuasarEngine/Asset/Asset.h>
#include <QuasarEngine/Resources/Materials/Material.h>

#include <QuasarEngine/Tools/Math.h>

#include <QuasarEngine/Renderer/VertexArray.h>
#include <QuasarEngine/Renderer/Buffer.h>
#include <QuasarEngine/Renderer/DrawMode.h>

namespace QuasarEngine
{
	class Mesh : public Asset
	{
	private:
		std::shared_ptr<VertexArray> m_vertexArray;
		std::shared_ptr<VertexBuffer> m_vertexBuffer;

		DrawMode m_drawMode;

		std::vector<float> m_vertices;
		std::vector<unsigned int> m_indices;

		glm::vec3 m_boundingBoxSize;
		glm::vec3 m_boundingBoxPosition;

		bool m_meshGenerated = false;

		std::optional<MaterialSpecification> m_material;

		void CalculateBoundingBoxSize(std::vector<float> vertices);
	public:
		Mesh(std::vector<float> vertices,
			std::vector<unsigned int> indices,
			std::optional<BufferLayout> layout,
			DrawMode drawMode = DrawMode::TRIANGLES,
			std::optional<MaterialSpecification> material = std::nullopt);
		~Mesh();

		void draw() const;

		std::optional<MaterialSpecification> GetMaterial() const { return m_material; }

		void GenerateMesh(std::vector<float> vertices, std::vector<unsigned int> indices, std::optional<BufferLayout> layout);

		glm::vec3 GetBoundingBoxSize() const { return m_boundingBoxSize; }

		bool IsVisible(const Math::Frustum& frustum, const glm::mat4& modelMatrix, const glm::vec3 size) const;

		bool IsMeshGenerated();

		void Clear();

		const size_t& GetVerticesCount() { return m_vertices.size(); }
		const size_t& GetIndicesCount() { return m_indices.size(); }

		const std::vector<float>& GetVertices() const { return m_vertices; }
		const std::vector<unsigned int>& GetIndices() const { return m_indices; }

		static AssetType GetStaticType() { return AssetType::MESH; }
		virtual AssetType GetType() override { return GetStaticType(); }
	};
}