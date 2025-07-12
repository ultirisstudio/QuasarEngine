#include "qepch.h"
#include "Registry.h"

namespace QuasarEngine
{
	Registry::Registry() : m_Registry{ nullptr }
	{
		m_Registry = std::make_unique<entt::registry>();
	}
}