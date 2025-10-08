#include "qepch.h"
#include "MeshComponent.h"

#include <utility>

namespace QuasarEngine
{
    MeshComponent::MeshComponent()
        : m_Mesh(nullptr), m_Name(""), m_ModelPath(""), m_NodePath(""), m_OwnsMesh(false)
    {
    }

    MeshComponent::MeshComponent(std::string name, Mesh* mesh, std::string modelPath)
        : m_Mesh(mesh), m_Name(std::move(name)), m_ModelPath(std::move(modelPath)), m_NodePath(""), m_OwnsMesh(false)
    {
    }

    MeshComponent::MeshComponent(std::string name)
        : m_Mesh(nullptr), m_Name(std::move(name)), m_ModelPath(""), m_NodePath(""), m_OwnsMesh(false)
    {
    }

    MeshComponent::~MeshComponent()
    {
        releaseOwnedMesh();
        m_Mesh = nullptr;
    }

    MeshComponent::MeshComponent(MeshComponent&& other) noexcept
    {
        moveFrom(other);
    }

    MeshComponent& MeshComponent::operator=(MeshComponent&& other) noexcept
    {
        if (this != &other)
        {
            releaseOwnedMesh();
            moveFrom(other);
        }
        return *this;
    }

    void MeshComponent::releaseOwnedMesh()
    {
        if (m_OwnsMesh && m_Mesh)
        {
            delete m_Mesh;
        }
        m_OwnsMesh = false;
        m_Mesh = nullptr;
    }

    void MeshComponent::moveFrom(MeshComponent& other) noexcept
    {
        m_Mesh = other.m_Mesh;
        m_OwnsMesh = other.m_OwnsMesh;
        m_Name = std::move(other.m_Name);
        m_ModelPath = std::move(other.m_ModelPath);
        m_NodePath = std::move(other.m_NodePath);
        m_LocalNodeTransform = std::move(other.m_LocalNodeTransform);

        other.m_Mesh = nullptr;
        other.m_OwnsMesh = false;
        other.m_LocalNodeTransform.reset();
    }

    void MeshComponent::ClearMesh() const
    {
        if (m_Mesh) m_Mesh->Clear();
    }

    void MeshComponent::GenerateMesh(std::vector<float>& vertices,
        std::vector<unsigned int>& indices,
        std::optional<BufferLayout> layout,
        DrawMode drawMode)
    {
        releaseOwnedMesh();

        m_Mesh = new Mesh(vertices, indices, layout, drawMode);
        m_OwnsMesh = true;
    }
}