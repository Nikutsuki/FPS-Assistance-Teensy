#include "bezier_engine.h"

void BezierEngine::init()
{
}

void BezierEngine::init_cubic_bezier(BezierPoint p0, BezierPoint p1, BezierPoint p2, BezierPoint p3) noexcept
{
	this->P0 = p0;
	this->P1 = p1;
	this->P2 = p2;
	this->P3 = p3;
}

BezierPoint BezierEngine::get_cubic_bezier_point(float t) noexcept
{
	BezierPoint p;
	p.x = pow(1 - t, 3) * P0.x + 3 * pow(1 - t, 2) * t * P1.x + 3 * (1 - t) * pow(t, 2) * P2.x + pow(t, 3) * P3.x;
	p.y = pow(1 - t, 3) * P0.y + 3 * pow(1 - t, 2) * t * P1.y + 3 * (1 - t) * pow(t, 2) * P2.y + pow(t, 3) * P3.y;
	return p;
}

std::vector<BezierPoint> BezierEngine::split_bezier_curve(int steps) noexcept
{
	std::vector<BezierPoint> points;
	std::vector<BezierPoint> points2;
	for (int i = 0; i < steps; i++)
	{
		points.push_back(get_cubic_bezier_point(static_cast<float>(i) / (steps - 1)));
	}
	for (int i = 0; i < steps - 1; i++)
	{
		points2.push_back(points[i + 1] - points[i]);
	}
	return points2;
}
