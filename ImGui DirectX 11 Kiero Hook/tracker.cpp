#include "tracker.h"

void Tracker::init() noexcept
{
	float fps = config->NN_FPS;
	tracker_instance = byte_track::BYTETracker(fps, 30, 0.5, 0.6, 0.8);
}

void Tracker::update(const std::vector<Detection>& detections) noexcept
{
	std::vector<byte_track::Object> objects;
	for (const auto& detection : detections)
	{
		byte_track::Rect<float> rect(detection.box.x, detection.box.y, detection.box.width, detection.box.height);

		byte_track::Object object(rect, detection.class_id, detection.confidence);
		objects.push_back(object);
	}

	tracker_instance.setHighThresh(config->trackerHighThreshold);
	tracker_instance.setMatchThresh(config->trackerMatchThreshold);
	tracker_instance.setMaxTimeLost(config->trackerFps, config->trackerTrackBuffer);

	tracks_mutex.lock();
	tracks = tracker_instance.update(objects);
	tracks_mutex.unlock();
}