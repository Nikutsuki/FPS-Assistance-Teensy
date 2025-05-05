#pragma once
#include "includes.h"
#include "Globals.h"

#include "s_track.h"
#include "object.h"
#include "lapjv.h"
#include "rect.h"

class Tracker : public virtual TrackerInstance
{
public:
	void init() noexcept override;
	void update(const std::vector<Detection>& detections) noexcept override;
};