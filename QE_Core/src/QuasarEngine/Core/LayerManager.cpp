#include "LayerManager.h"

LayerManager::~LayerManager()
{
		
}

void LayerManager::PushLayer(Layer* layer)
{
	m_Layers.emplace(m_Layers.begin() + m_LayerIndex, layer);
	m_LayerIndex++;
}

void LayerManager::PushOverlay(Layer* overlay)
{
	m_Layers.emplace_back(overlay);
}