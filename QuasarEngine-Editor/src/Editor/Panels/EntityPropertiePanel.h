#pragma once

#include "../SceneManager.h"

#include "SceneHierarchy.h"

#include "ComponentPanels/TransformComponentPanel.h"
#include "ComponentPanels/CameraComponentPanel.h"
#include "ComponentPanels/MeshComponentPanel.h"
#include "ComponentPanels/TerrainComponentPanel.h"
#include "ComponentPanels/MaterialComponentPanel.h"
#include "ComponentPanels/LightComponentPanel.h"
#include "ComponentPanels/MeshRendererComponentPanel.h"
#include "ComponentPanels/Physics/RigidBodyComponentPanel.h"
#include "ComponentPanels/Physics/BoxColliderComponentPanel.h"
#include "ComponentPanels/Physics/MeshColliderComponentPanel.h"
#include "ComponentPanels/Physics/CapsuleColliderComponentPanel.h"
#include "ComponentPanels/Physics/SphereColliderComponentPanel.h"

namespace QuasarEngine
{
	class EntityPropertiePanel
	{
	public:
		EntityPropertiePanel();
		~EntityPropertiePanel();

		void OnImGuiRender(Scene& scene, SceneHierarchy& sceneHierarchy);
	private:
		std::unique_ptr<TransformComponentPanel> m_TransformComponentPanel;
		std::unique_ptr<CameraComponentPanel> m_CameraComponentPanel;
		std::unique_ptr<MeshComponentPanel> m_MeshComponentPanel;
		std::unique_ptr<TerrainComponentPanel> m_TerrainComponentPanel;
		std::unique_ptr<MaterialComponentPanel> m_MaterialComponentPanel;
		std::unique_ptr<LightComponentPanel> m_LightComponentPanel;
		std::unique_ptr<MeshRendererComponentPanel> m_MeshRendererComponentPanel;
		std::unique_ptr<RigidBodyComponentPanel> m_RigidBodyComponentPanel;
		std::unique_ptr<BoxColliderComponentPanel> m_BoxColliderComponentPanel;
		std::unique_ptr<MeshColliderComponentPanel> m_MeshColliderComponentPanel;
		std::unique_ptr<CapsuleColliderComponentPanel> m_CapsuleColliderComponentPanel;
		std::unique_ptr<SphereColliderComponentPanel> m_SphereColliderComponentPanel;
	};
}