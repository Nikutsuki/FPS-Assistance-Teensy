#pragma once
#include "includes.h"
#include "Globals.h"

class Aimbot : public virtual AimbotInstance
{
	void loop() noexcept override;
	void test_aimbot() noexcept override;
};