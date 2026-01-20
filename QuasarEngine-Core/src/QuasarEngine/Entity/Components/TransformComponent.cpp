#include "qepch.h"
#include "TransformComponent.h"
#include "HierarchyComponent.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Renderer/Renderer.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>

namespace QuasarEngine
{
	TransformComponent::TransformComponent(const glm::vec3& position) : Position(position) {}

	glm::mat4 TransformComponent::GetLocalTransform() const
	{
		//glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(Rotation.x), { 1, 0, 0 }) * glm::rotate(glm::mat4(1.0f), glm::radians(Rotation.y), { 0, 1, 0 }) * glm::rotate(glm::mat4(1.0f), glm::radians(Rotation.z), {0, 0, 1});
		//return glm::translate(glm::mat4(1.0f), Position) * rotation * glm::scale(glm::mat4(1.0f), Scale);
		return glm::translate(glm::mat4(1.0f), Position) * glm::toMat4(glm::quat(Rotation)) * glm::scale(glm::mat4(1.0f), Scale);
	}

	glm::mat4 TransformComponent::GetGlobalTransform() const
	{
		glm::mat4 globalTransform = GetLocalTransform();

		Entity entity{ entt_entity, registry };
		UUID parentID = (entity.HasComponent<HierarchyComponent>()) ? entity.GetComponent<HierarchyComponent>().m_Parent : UUID::Null();

		while (parentID != UUID::Null())
		{
			std::optional<Entity> parent = Renderer::Instance().m_SceneData.m_Scene->GetEntityByUUID(parentID);
			if (parent.has_value())
			{
				globalTransform = parent.value().GetComponent<TransformComponent>().GetLocalTransform() * globalTransform;
				parentID = (parent.value().HasComponent<HierarchyComponent>()) ? parent.value().GetComponent<HierarchyComponent>().m_Parent : UUID::Null();
			}
			else
			{
				break;
			}
		}

		return globalTransform;
	}

	glm::mat4 TransformComponent::CalculateViewMatrix(glm::mat4 transform) const
	{
		glm::vec3 m_target;
		glm::vec3 m_up;
		glm::vec3 m_right;

		glm::vec3 position, scale;
		glm::quat rotationQuat;
		glm::decompose(transform, scale, rotationQuat, position, glm::vec3(), glm::vec4());

		glm::vec3 front = rotationQuat * glm::vec3(0.0f, 0.0f, -1.0f);

		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		if (fabs(glm::abs(front.y)) >= 1.0f - FLT_EPSILON) {
			up = glm::normalize(glm::cross(front, glm::vec3(0.0f, 0.0f, 1.0f)));
		}

		glm::vec3 target = glm::normalize(front);
		glm::vec3 right = glm::normalize(glm::cross(target, up));

		return glm::lookAt(position, position + target, up);
	}

	glm::mat4 TransformComponent::GetLocalViewMatrix() const
	{
		//return glm::inverse(GetLocalTransform());
		return CalculateViewMatrix(GetLocalTransform());
	}

	glm::mat4 TransformComponent::GetGlobalViewMatrix() const
	{
		//return glm::inverse(GetGlobalTransform());
		return CalculateViewMatrix(GetGlobalTransform());
	}
}