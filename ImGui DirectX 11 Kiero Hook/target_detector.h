#pragma once
#include "includes.h"
#include "Globals.h"
#include "yolo.h"

#define M_PI 3.14159265358979323846
#define RADTODEG 180.0f / M_PI
#define DEGTORAD M_PI / 180.0f

class TargetDetector : public virtual TargetDetectorInstance
{
public:
	void init() override;
	void find_closest_target() override;
	int Full360() noexcept override;
	int GetCoordsX(int delta) noexcept override;
	int GetCoordsY(int delta) noexcept override;
	void update_target_position(int x, int y) noexcept override;
	int GetPixelsX(int delta) noexcept override;
	int GetPixelsY(int delta) noexcept override;
	int DegreesToPixelsX(float degrees) noexcept override;
	int DegreesToPixelsY(float degrees) noexcept override;
	void UpdateTarget(Target& target) noexcept override;
};