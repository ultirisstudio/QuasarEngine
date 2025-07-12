#pragma once

#include "Crosshair.h"
#include <memory>

class HUD
{
private:
	std::unique_ptr<Crosshair> m_Crosshair;

public:
	HUD();
	~HUD();

	void Render();
};

