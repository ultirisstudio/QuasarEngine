#include "qepch.h"
#include "MeshColliderComponent.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Physic/PhysicEngine.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Entity/Components/MeshComponent.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>

namespace QuasarEngine
{
	MeshColliderComponent::MeshColliderComponent()
	{
        
	}

    void MeshColliderComponent::Generate()
    {
        if (m_IsConvex)
        {
			GenerateConvexMesh();
		}
        else
        {
			GenerateConcaveMesh();
		}
    }

    void MeshColliderComponent::GenerateConvexMesh()
    {
        Entity entity{entt_entity, registry};
        auto& mc = entity.GetComponent<MeshComponent>();

        const int nbVertices = mc.GetMesh().GetVerticesCount();
        const int nbIndices = mc.GetMesh().GetIndicesCount();
        const int nbFaces = mc.GetMesh().GetIndicesCount() / 3;

        float* vertices = new float[nbVertices * 3];
        for (int v = 0; v < nbVertices; v++)
        {
            //vertices[v * 3] = mc.GetMesh().GetVertices()[v].position.x;
            //vertices[v * 3 + 1] = mc.GetMesh().GetVertices()[v].position.y;
            //vertices[v * 3 + 2] = mc.GetMesh().GetVertices()[v].position.z;
        }

        int* indices = new int[nbIndices];
        for (int i = 0; i < nbIndices; i++)
        {
            indices[i] = mc.GetMesh().GetIndices()[i];
        }

        /*reactphysics3d::PolygonVertexArray::PolygonFace* faces = new reactphysics3d::PolygonVertexArray::PolygonFace[nbFaces];
        for (int f = 0; f < nbFaces; f++)
        {
			faces[f].indexBase = f * 3;
			faces[f].nbVertices = 3;
		}

        reactphysics3d::PolygonVertexArray polygonVertexArray(nbVertices, vertices, 3 * sizeof(float), indices, 3 * sizeof(int), nbFaces, faces,
            reactphysics3d::PolygonVertexArray::VertexDataType::VERTEX_FLOAT_TYPE,
            reactphysics3d::PolygonVertexArray::IndexDataType::INDEX_INTEGER_TYPE);

        // Compute the convex mesh from the array of vertices
        std::vector<rp3d::Message> messages;
        reactphysics3d::ConvexMesh* convexMesh = PhysicEngine::GetPhysicsCommon()->createConvexMesh(polygonVertexArray, messages);

        // Display the messages (info, warning and errors)
        if (messages.size() > 0) {

            for (const rp3d::Message& message : messages) {

                std::string messageType;

                switch (message.type) {

                case rp3d::Message::Type::Information:
                    messageType = "info";
                    break;
                case rp3d::Message::Type::Warning:
                    messageType = "warning";
                    break;
                case rp3d::Message::Type::Error:
                    messageType = "error";
                    break;
                }

                std::cout << "Message (" << messageType << "): " << message.text << std::endl;
            }
        }

        assert(convexMesh != nullptr);

        const reactphysics3d::Vector3 scaling(1, 1, 1);

        m_convexMeshShape = PhysicEngine::GetPhysicsCommon()->createConvexMeshShape(convexMesh, scaling);

        if (entity.HasComponent<RigidBodyComponent>())
        {
            auto& rb = entity.GetComponent<RigidBodyComponent>();
            rb.GetRigidBody()->addCollider(m_convexMeshShape, reactphysics3d::Transform::identity());
        }*/
    }

    void MeshColliderComponent::GenerateConcaveMesh()
    {
        Entity entity{entt_entity, registry };
        auto& mc = entity.GetComponent<MeshComponent>();

        const int nbVertices = mc.GetMesh().GetVerticesCount();
        const int nbIndices = mc.GetMesh().GetIndicesCount();
        const int nbTriangles = mc.GetMesh().GetIndicesCount() / 3;

        float* vertices = new float[nbVertices * 3];
        for (int i = 0; i < nbVertices; i++)
        {
			//vertices[i * 3] = mc.GetMesh().GetVertices()[i].position.x;
			//vertices[i * 3 + 1] = mc.GetMesh().GetVertices()[i].position.y;
			//vertices[i * 3 + 2] = mc.GetMesh().GetVertices()[i].position.z;
		}

        /*float* normals = new float[nbVertices * 3];
        for (int i = 0; i < nbVertices; i++)
        {
            vertices[i * 3] = mc.GetMesh().GetVertices()[i].normal.x;
            vertices[i * 3 + 1] = mc.GetMesh().GetVertices()[i].normal.y;
            vertices[i * 3 + 2] = mc.GetMesh().GetVertices()[i].normal.z;
        }*/

        int* indices = new int[nbIndices];
        for (int i = 0; i < nbIndices; i++)
        {
			indices[i] = mc.GetMesh().GetIndices()[i];
		}

        /*reactphysics3d::TriangleVertexArray* triangleVertexArray =
            new reactphysics3d::TriangleVertexArray(nbVertices, vertices, 3 * sizeof(float), nbTriangles, indices, 3 * sizeof(int),
                reactphysics3d::TriangleVertexArray::VertexDataType::VERTEX_FLOAT_TYPE,
                reactphysics3d::TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE);*/

        
        /*reactphysics3d::TriangleVertexArray* triangleVertexArray =
            new reactphysics3d::TriangleVertexArray(nbVertices, vertices, 3 * sizeof(float), normals, 3 * sizeof(float), nbTriangles, indices, 3 * sizeof(int),
                reactphysics3d::TriangleVertexArray::VertexDataType::VERTEX_FLOAT_TYPE,
                reactphysics3d::TriangleVertexArray::NormalDataType::NORMAL_FLOAT_TYPE,
                reactphysics3d::TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE);*/
        

        /*std::vector<rp3d::Message> messages;
        reactphysics3d::TriangleMesh* triangleMesh = PhysicEngine::GetPhysicsCommon()->createTriangleMesh(*triangleVertexArray, messages);

        if (messages.size() > 0) {

            for (const rp3d::Message& message : messages) {

                std::string messageType;
                switch (message.type) {
                case rp3d::Message::Type::Information:
                    messageType = "info";
                    break;
                case rp3d::Message::Type::Warning:
                    messageType = "warning";
                    break;
                case rp3d::Message::Type::Error:
                    messageType = "error";
                    break;
                }

                std::cout << "Message (" << messageType << "): " << message.text << std::endl;
            }
        }

        assert(triangleMesh != nullptr);

        glm::vec3 scale = entity.GetComponent<TransformComponent>().Scale;
        const reactphysics3d::Vector3 scaling(scale.x, scale.y, scale.z);

        m_concaveMeshShape = PhysicEngine::GetPhysicsCommon()->createConcaveMeshShape(triangleMesh, scaling);

        if (entity.HasComponent<RigidBodyComponent>())
        {
            auto& rb = entity.GetComponent<RigidBodyComponent>();
            rb.GetRigidBody()->addCollider(m_concaveMeshShape, reactphysics3d::Transform::identity());
        }*/
    }
}