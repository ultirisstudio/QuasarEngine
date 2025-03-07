#pragma once

#include <string>

class BlockEvent
{
public:
	virtual std::string toString() const {
		return "";
	}

	virtual std::string printInfos() const {
		return "";
	}
};