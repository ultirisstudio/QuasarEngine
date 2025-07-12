#pragma once

#include <memory>
#include <glm/glm.hpp>

#include <QuasarEngine/Shader/Shader.h>
#include <QuasarEngine/Resources/Mesh.h>

class Crosshair
{
public:
	Crosshair();
	~Crosshair() = default;

	void Render();
private:
	//std::unique_ptr<QuasarEngine::Shader> m_Shader;
	std::unique_ptr<QuasarEngine::Mesh> m_Mesh;
};