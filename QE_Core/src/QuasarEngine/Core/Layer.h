#pragma once

#include <string>

class Layer {
public:
	Layer(const std::string& name = "Layer");
	virtual ~Layer() = default;

	virtual void OnAttach() {}
	virtual void OnDetach() {}
	virtual void OnUpdate(double dt) {}
protected:
	std::string m_Name;
};