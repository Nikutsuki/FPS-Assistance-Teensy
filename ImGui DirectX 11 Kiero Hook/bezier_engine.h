#pragma once
#include "includes.h"
#include "Globals.h"

class BezierEngine : public virtual BezierEngineInstance
{
	void init() override;
	void init_cubic_bezier(BezierPoint p0, BezierPoint p1, BezierPoint p2, BezierPoint p3) noexcept override;
	BezierPoint get_cubic_bezier_point(float t) noexcept override;
	std::vector<BezierPoint> split_bezier_curve(int steps) noexcept override;
};