#pragma once

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Scene/Scene.h>

namespace QuasarEngine
{
	class SceneHierarchy
	{
	public:
		SceneHierarchy();

		void OnImGuiRender(Scene& scene);

		Entity m_SelectedEntity;
	private:
		//void OnDrawEntityNode(Scene& scene, Entity entity, int count);
		void OnDrawEntityNode(Scene& scene, Entity entity);
	};
}