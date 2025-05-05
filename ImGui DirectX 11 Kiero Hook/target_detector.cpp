#include "target_detector.h"

int TargetDetector::Full360() noexcept
{
	return static_cast<int>(emulation->ads_on ? Full360val : (Full360val * 8.f / 10.f));
}

int TargetDetector::GetCoordsX(int delta) noexcept
{
	constexpr int total = SCREEN_WIDTH;
	const double lookAt = delta * 2.0 / total;
	const double degrees = atan(lookAt * tan((emulation->ads_on ? 41.5f : 52.0f) * DEGTORAD)) * RADTODEG;
	return static_cast<int>((Full360() * degrees) / 360);
}

int TargetDetector::GetCoordsY(int delta) noexcept
{
	constexpr int total = SCREEN_HEIGHT;
	const double lookAt = delta * 2.0 / total;
	const double degrees = atan(lookAt * tan((emulation->ads_on ? 26.5f : 36.f) * DEGTORAD)) * RADTODEG;
	return static_cast<int>((Full360() * degrees) / 360);
}

void TargetDetector::update_target_position(int x, int y) noexcept
{
	std::lock_guard<std::mutex> lock(targets_mutex);
	for (auto& target : *targets)
	{
		target.x_pixels -= GetPixelsX(x);
		target.y_pixels -= GetPixelsY(y);
		target.x_movement = GetCoordsX(target.x_pixels);
		target.y_movement = GetCoordsY(target.y_pixels);
	}
}

int TargetDetector::GetPixelsX(int delta) noexcept
{
	constexpr int total = SCREEN_WIDTH;
	const double degrees = (delta * 360.0) / Full360();
	const double tanAngle = tan((emulation->ads_on ? 41.5f : 52.0f) * DEGTORAD);
	const double lookAt = tan(degrees * DEGTORAD) / tanAngle;

	return static_cast<int>((lookAt * total) / 2.0);
}

int TargetDetector::GetPixelsY(int delta) noexcept
{
	constexpr int total = SCREEN_HEIGHT;
	const double degrees = (delta * 360.0) / Full360();
	const double tanAngle = tan((emulation->ads_on ? 26.5f : 36.f) * DEGTORAD);
	const double lookAt = tan(degrees * DEGTORAD) / tanAngle;

	return static_cast<int>((lookAt * total) / 2.0);
}

int TargetDetector::DegreesToPixelsX(float degrees) noexcept
{
	constexpr int total = SCREEN_WIDTH;
	const double tanValue = tan((emulation->ads_on ? 26.5f : 36.f) * DEGTORAD);
	const double lookAt = tan(degrees * DEGTORAD) / tanValue;
	const double delta = lookAt * total / 2.0;
	return static_cast<int>(delta);
}

int TargetDetector::DegreesToPixelsY(float degrees) noexcept
{
	constexpr int total = SCREEN_HEIGHT;
	const double tanValue = tan((emulation->ads_on ? 41.5f : 52.0f) * DEGTORAD);
	const double lookAt = tan(degrees * DEGTORAD) / tanValue;
	const double delta = lookAt * total / 2.0;
	return static_cast<int>(delta);
}

void TargetDetector::UpdateTarget(Target& target) noexcept
{
	target.x_movement = GetCoordsX(target.x_pixels);
	target.y_movement = GetCoordsY(target.y_pixels);

	const auto distance = sqrt(target.x_pixels * target.x_pixels + target.y_pixels * target.y_pixels);

	target.distance = distance;
}

static float rectangle_sdf(ImVec2 center, ImVec2 half_size, ImVec2 point_in)
{
	const ImVec2 point = ImVec2(abs(point_in.x - center.x), abs(point_in.y - center.y));

	const float x_dist = point.x - half_size.x;
	const float y_dist = point.y - half_size.y;

	const ImVec2 dist_vec = ImVec2(point.x - half_size.x, point.y - half_size.y);

	const float c_dist = sqrt(dist_vec.x * dist_vec.x + dist_vec.y * dist_vec.y);

	return x_dist > 0.0 && y_dist > 0.0 ? c_dist : std::max(x_dist, y_dist);
}

void TargetDetector::init()
{
	this->targets = std::make_shared<std::vector<Target>>();
}

void TargetDetector::find_closest_target()
{
	std::vector<Track> detections;

	{
		std::lock_guard<std::mutex> lock(tracker->tracks_mutex);
		detections = tracker->tracks;
	}

	{
		std::lock_guard<std::mutex> lock(targets_mutex);
		targets->clear();
	}

	if (detections.size() == 0)
	{
		closest_target_x = 0;
		closest_target_y = 0;
		target_found = false;
		return;
	}

	constexpr int crosshair_x = NEURAL_NETWORK_INPUT_X / 2;
	constexpr int crosshair_y = NEURAL_NETWORK_INPUT_Y / 2;

	for (int i = 0; i < detections.size(); i++)
	{
		const Track& detection = gsl::at(detections, i);
		Target target;
		float distance = 10000000.f;
		switch (detection->getClassId())
		{
		case 0:
			target.x_pixels = static_cast<int>(detection->getRect().x() + (detection->getRect().width() * 0.5f) - crosshair_x);
			target.y_pixels = static_cast<int>(detection->getRect().y() + (detection->getRect().height() * 0.1f) - crosshair_y);
			target.x_movement = GetCoordsX(target.x_pixels);
			target.y_movement = GetCoordsY(target.y_pixels);
			target.target_type = BODY;
			target.priority = 0;
			distance = rectangle_sdf(ImVec2(detection->getRect().x() + (detection->getRect().width() / 2.f), detection->getRect().y() + (detection->getRect().height() / 2.f)), ImVec2(detection->getRect().width() / 2.f, detection->getRect().height() / 2.f), ImVec2(crosshair_x, crosshair_y));
			target.target_id = detection->getTrackId();
			if (distance <= 0) target.is_on_body = true;
			break;
		case 1:
			target.x_pixels = static_cast<int>(detection->getRect().x() + (detection->getRect().width() * 0.5f) - crosshair_x);
			target.y_pixels = static_cast<int>(detection->getRect().y() + (detection->getRect().height() * 0.5f) - crosshair_y);
			target.x_movement = GetCoordsX(target.x_pixels);
			target.y_movement = GetCoordsY(target.y_pixels);
			target.target_type = HEAD;
			target.priority = 0;
			distance = rectangle_sdf(ImVec2(detection->getRect().x() + (detection->getRect().width() / 2.f), detection->getRect().y() + (detection->getRect().height() / 2.f)), ImVec2(detection->getRect().width() / 2.f, detection->getRect().height() / 2.f), ImVec2(crosshair_x, crosshair_y));
			target.target_id = detection->getTrackId();
			if (distance <= 0) target.is_on_head = true;
			break;
		case 2:
			target.x_pixels = static_cast<int>(detection->getRect().x() + (detection->getRect().width() * 0.5f) - crosshair_x);
			target.y_pixels = static_cast<int>(detection->getRect().y() + (detection->getRect().height() * 0.5f) - crosshair_y);
			target.x_movement = GetCoordsX(target.x_pixels);
			target.y_movement = GetCoordsY(target.y_pixels);
			target.target_type = ISO_BALL;
			target.priority = 0;
			distance = rectangle_sdf(ImVec2(detection->getRect().x() + (detection->getRect().width() / 2.f), detection->getRect().y() + (detection->getRect().height() / 2.f)), ImVec2(detection->getRect().width() / 2.f, detection->getRect().height() / 2.f), ImVec2(crosshair_x, crosshair_y));
			target.target_id = detection->getTrackId();
			if (distance <= 0) target.is_on_head = true;
			break;
		case 3:
			target.x_pixels = static_cast<int>(detection->getRect().x() + (detection->getRect().width() * 0.5f) - crosshair_x);
			target.y_pixels = static_cast<int>(detection->getRect().y() + (detection->getRect().height() * 0.5f) - crosshair_y);
			target.x_movement = GetCoordsX(target.x_pixels);
			target.y_movement = GetCoordsY(target.y_pixels);
			target.target_type = REYNA_FLASH;
			target.priority = 0;
			distance = rectangle_sdf(ImVec2(detection->getRect().x() + (detection->getRect().width() / 2.f), detection->getRect().y() + (detection->getRect().height() / 2.f)), ImVec2(detection->getRect().width() / 2.f, detection->getRect().height() / 2.f), ImVec2(crosshair_x, crosshair_y));
			target.target_id = detection->getTrackId();
			if (distance <= 0) target.is_on_head = true;
			break;
		default:
			break;
		}

		{
			std::lock_guard<std::mutex> lock(targets_mutex);
			targets->push_back(target);
		}
	}

	{
		std::lock_guard<std::mutex> lock(targets_mutex);
		for (auto& target : *targets)
		{
			const auto distance = sqrt(target.x_pixels * target.x_pixels + target.y_pixels * target.y_pixels);
			if (target.target_type == ISO_BALL || target.target_type == REYNA_FLASH) target.priority += 3;
			if (target.target_type == HEAD) target.priority += 2;
			if (abs(target.x_pixels) <= 10 && abs(target.y_pixels) <= 10) target.priority += 2;
			target.distance = distance;
		}
	}

	extrapolator->update(*targets);

	this->aimbot_new_target_found = true;
}