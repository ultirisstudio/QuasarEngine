#pragma once

#include <Editor/Modules/IEditorModule.h>
#include <QuasarEngine/Entity/Entity.h>

namespace QuasarEngine
{
	class Scene;

	class SceneHierarchy : public IEditorModule
	{
	public:
		SceneHierarchy(EditorContext& context);
		~SceneHierarchy() override;

		void Update(double dt) override;
		void RenderUI() override;
	private:
		void OnDrawEntityNode(Entity entity, int depth = 0);
	};
}