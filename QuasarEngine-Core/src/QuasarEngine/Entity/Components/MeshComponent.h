#pragma once

#include <QuasarEngine/Entity/Component.h>
#include <QuasarEngine/Resources/Mesh.h>

namespace QuasarEngine
{
	class MeshComponent : public Component
	{
	public:
		MeshComponent();
		MeshComponent(std::string name, Mesh* mesh, std::string modelPath);
		~MeshComponent() override;

		Mesh& GetMesh() const { return *m_Mesh; }
		std::string GetName() { return m_Name; }
		std::string GetModelPath() { return m_ModelPath; }

		bool HasMesh() const { return m_Mesh != nullptr; }

		void ClearMesh() const;

		void GenerateMesh(std::vector<float>& vertices, std::vector<unsigned int>& indices, std::optional<BufferLayout> layout = {}, DrawMode drawMode = DrawMode::TRIANGLES);

		Mesh* m_Mesh;
		std::string m_Name;
		std::string m_ModelPath;
	};
}