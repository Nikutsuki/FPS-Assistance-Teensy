#pragma once
#include "includes.h"
#include "Globals.h"
#include "yolo.h"
#include "precise_sleep.h"
#include "aim_correction.h"

class AimAssist : public virtual AimAssistInstance
{
	void loop(int& x, int& y) noexcept override;
	void select_func() noexcept override;
};