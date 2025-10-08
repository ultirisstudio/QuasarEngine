#pragma once

#include <optional>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <QuasarEngine/Entity/Component.h>
#include <QuasarEngine/Resources/Mesh.h>
#include <QuasarEngine/Renderer/DrawMode.h>
#include <QuasarEngine/Renderer/Buffer.h>

namespace QuasarEngine
{
    class MeshComponent : public Component
    {
    public:
        MeshComponent();
        MeshComponent(std::string name, Mesh* mesh, std::string modelPath);
        explicit MeshComponent(std::string name);
        ~MeshComponent() override;

        MeshComponent(const MeshComponent&) = delete;
        MeshComponent& operator=(const MeshComponent&) = delete;

        MeshComponent(MeshComponent&& other) noexcept;
        MeshComponent& operator=(MeshComponent&& other) noexcept;

        Mesh& GetMesh() const { return *m_Mesh; }
        bool HasMesh() const { return m_Mesh != nullptr; }

        const std::string& GetName() const { return m_Name; }
        void SetName(std::string name) { m_Name = std::move(name); }

        const std::string& GetModelPath() const { return m_ModelPath; }
        void SetModelPath(std::string p) { m_ModelPath = std::move(p); }

        const std::string& GetNodePath() const { return m_NodePath; }
        void SetNodePath(std::string path) { m_NodePath = std::move(path); }

        bool HasLocalNodeTransform() const { return m_LocalNodeTransform.has_value(); }
        const glm::mat4& GetLocalNodeTransform() const
        {
            static const glm::mat4 I(1.0f);
            return m_LocalNodeTransform ? *m_LocalNodeTransform : I;
        }
        void SetLocalNodeTransform(const glm::mat4& t) { m_LocalNodeTransform = t; }
        void ClearLocalNodeTransform() { m_LocalNodeTransform.reset(); }

        void ClearMesh() const;
        void GenerateMesh(std::vector<float>& vertices,
            std::vector<unsigned int>& indices,
            std::optional<BufferLayout> layout = {},
            DrawMode drawMode = DrawMode::TRIANGLES);

        void SetMesh(Mesh* mesh) { releaseOwnedMesh(); m_Mesh = mesh; m_OwnsMesh = false; }

        Mesh* m_Mesh;

    private:
        void releaseOwnedMesh();
        void moveFrom(MeshComponent& other) noexcept;

        std::string m_Name;
        std::string m_ModelPath;
        std::string m_NodePath;

        std::optional<glm::mat4> m_LocalNodeTransform;

        bool m_OwnsMesh = false;
    };
}