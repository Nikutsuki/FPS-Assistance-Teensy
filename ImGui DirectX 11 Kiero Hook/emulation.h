#pragma once
#include "includes.h"
#include "Globals.h"
#include "aim_correction.h"

class Emulation : public virtual EmulationInstance
{
public:
	int init() override;
	void loop() override;
	void shoot_trigger() noexcept override;
	void aimbot2(int x, int y) noexcept override;
};