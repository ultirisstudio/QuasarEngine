#include "qepch.h"
#include "QuasarEngine/Renderer/RenderCommand.h"

namespace QuasarEngine {

	std::unique_ptr<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();

}