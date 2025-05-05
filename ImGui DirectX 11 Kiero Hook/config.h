#pragma once
#include "includes.h"
#include "Globals.h"

class Config : public virtual ConfigInstance
{
	void addDebugMessage(std::string message) override;
};