#include "qepch.h"
#include "MaterialComponent.h"

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Shader/Shader.h>

namespace QuasarEngine
{
	MaterialComponent::MaterialComponent()
	{
		MaterialSpecification specification;
		m_Material = Material::CreateMaterial(specification);

		//Renderer::Instance().m_SceneData.m_Shader->AcquireResources(m_Material.get());
	}

	MaterialComponent::MaterialComponent(const MaterialSpecification& specification)
	{
		m_Material = Material::CreateMaterial(specification);

		//Renderer::Instance().m_SceneData.m_Shader->AcquireResources(m_Material.get());
	}

	MaterialComponent::~MaterialComponent()
	{
		//Renderer::Instance().m_SceneData.m_Shader->ReleaseResources(m_Material.get());

		m_Material.reset();
	}

	Material& MaterialComponent::GetMaterial()
	{
		return *m_Material;
	}
}