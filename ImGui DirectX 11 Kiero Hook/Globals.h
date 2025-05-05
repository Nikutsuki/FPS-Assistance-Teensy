#pragma once
#include "includes.h"

#include "byte_tracker.h"
#include "rect.h"
#include "s_track.h"

constexpr int SCREEN_WIDTH = 1920;
constexpr int SCREEN_HEIGHT = 1080;
constexpr int NEURAL_NETWORK_INPUT_X = 320;
constexpr int NEURAL_NETWORK_INPUT_Y = 320;

enum TARGET_TYPE
{
	BODY = 0,
	HEAD = 1,
	ISO_BALL = 2,
	REYNA_FLASH = 3
};

struct Target
{
	TARGET_TYPE target_type = BODY;
	int x_movement = 0;
	int y_movement = 0;
	int x_pixels = 0;
	int y_pixels = 0;
	bool is_on_head = false;
	bool is_on_body = false;
	float priority = 0.f;
	float distance = 0.f;
	int target_id = 0;
};

struct BezierPoint
{
	float x;
	float y;

	inline BezierPoint operator+(const BezierPoint& other) const noexcept
	{
		return { x + other.x, y + other.y };
	}

	inline BezierPoint operator-(const BezierPoint& other) const noexcept
	{
		return { x - other.x, y - other.y };
	}
};

class EmulationInstance
{
public:
	virtual ~EmulationInstance() = default;
	virtual int init() = 0;
	virtual void loop() = 0;
	virtual void shoot_trigger() = 0;
	virtual void aimbot2(int x, int y) = 0;
	char buffer1[6];
	char buffer2[6];
	SOCKET clientSocket1;
	SOCKET clientSocket2;
	bool data_sent = false;
	bool data_received = false;
	bool trigger_on = false;
	bool ads_on = false;
	bool trigger_should_shoot = false;
	bool mouse_button_down = false;
	bool trigger_should_reset = 0;
	bool side_button_down = false;
	int trigger_last_time = 0;
};

class DebugWindowInstance
{
	virtual void addDebugMessage(char* message) = 0;
	std::deque<char* > debug_messages;
};

class ConfigInstance
{
public:
	virtual ~ConfigInstance() = default;
	float HUE_MIN = 57.0f;
	float HUE_MAX = 63.0f;
	float SATURATION_MIN = 0.3f;
	float SATURATION_MAX = 0.8f;
	float VALUE_MIN = 0.3f;
	float VALUE_MAX = 0.8f;
	int PIXELBOX_SIZE = 4;
	bool FILL = false;
	int FILL_ITERATIONS = 4;
	int MIN_DELAY = 1;
	int MAX_DELAY = 1;
	int AIM_ASSIST_TIME = 400;
	int AIMBOT_TIME = 100;
	bool AIMBOT_ON = false;
	bool AIM_ASSIST_ON = false;
	float NN_FPS = 60.0f;
	float TRIGGER_BOT_FPS = 120.0f;
	float AIMBOT_FOV = 5.f;
	float AIM_ASSIST_FOV = 20.f;
	bool DRAW_DETECTION_RECTANGLES = false;
	bool DRAW_AIM_ASSIST = false;
	bool DRAW_AIMBOT = false;
	bool DRAW_AIMASSIST_TARGET = false;
	bool DRAW_AIMBOT_TARGET = false;
	int AIMBOT_DAMPENING = 0;
	int AIM_ASSIST_DAMPENING = 0;
	int EXTRAPOLATION_BUFFER = 5;
	bool TOGGLE_EXTRAPOLATION_AIMBOT = false;
	bool TOGGLE_EXTRAPOLATION_AIMASSIST = true;
	std::chrono::duration<double> NN_RUN_TIME;
	int EXTRAPOLATION_OFFSET = 0;
	std::deque<std::string> debug_messages;
	//int aimbot_mode = 2;
	int aim_assist_mode = 2;
	//const char* aimbot_modes[5] = { "Linear", "Logarithmic", "Square", "Sigmoid", "X / 1 + X" };
	const char* aim_assist_modes[5] = { "Linear", "Logarithmic", "Square", "Sigmoid", "X / 1 + X"};
	float modelHeadConfidenceThreshold{ 0.5f };
	float modelBodyConfidenceThreshold{ 0.8f };
	float modelIsoBallConfidenceThreshold{ 0.8f };
	float modelReynaFlashConfidenceThreshold{ 0.8f };
	float trackerHighThreshold{ 0.6f };
	float trackerMatchThreshold{ 0.8f };
	float trackerFps{ 60.0f };
	int trackerTrackBuffer{ 30 };
	virtual void addDebugMessage(std::string message) = 0;
};

class AimAssistInstance
{
public:
	virtual ~AimAssistInstance() = default;
	virtual void loop(int& x, int& y) = 0;
	virtual void select_func() = 0;
	float (*aimbot_func)(float) = nullptr;
	int target_type = 0;
	int x_movement = 0;
	int y_movement = 0;
	bool can_pull = false;
	Target target = {};
};

class AimbotInstance
{
public:
	virtual ~AimbotInstance() = default;
	virtual void loop() = 0;
	virtual void test_aimbot() = 0;
	float (*aimbot_func)(float) = nullptr;
	int target_type = 0;
	Target target = {};
	bool is_in_motion = false;
	int step = 0;
	std::vector<BezierPoint> aimbot_points;
	int cached_x = 0;
	int cached_y = 0;
	int movement_done_x = 0;
	int movement_done_y = 0;
	int total_x = 0;
	int total_y = 0;
};

struct Detection
{
	float confidence;
	cv::Rect box;
	int class_id;
	std::string class_name;
};

class TargetDetectorInstance
{
public:
	virtual ~TargetDetectorInstance() = default;
	virtual void init() = 0;
	virtual void find_closest_target() = 0;
	virtual int Full360() = 0;
	virtual int GetCoordsX(int delta) = 0;
	virtual int GetCoordsY(int delta) = 0;
	virtual int GetPixelsX(int delta) = 0;
	virtual int GetPixelsY(int delta) = 0;
	virtual void update_target_position(int x, int y) = 0;
	virtual int DegreesToPixelsX(float degrees) = 0;
	virtual int DegreesToPixelsY(float degrees) = 0;
	virtual void UpdateTarget(Target& target) = 0;
	std::shared_ptr<std::vector<Target>> targets;
	std::mutex targets_mutex;
	int closest_target_x = 0;
	int closest_target_y = 0;
	float closest_target_area = 0;
	bool target_found = false;
	int Full360val = 102856;
	bool aimbot_new_target_found = false;
};

class BezierEngineInstance
{
public:
	virtual ~BezierEngineInstance() = default;
	virtual void init() = 0;
	virtual void init_cubic_bezier(BezierPoint p0, BezierPoint p1, BezierPoint p2, BezierPoint p3) = 0;
	virtual BezierPoint get_cubic_bezier_point(float t) = 0;
	virtual std::vector<BezierPoint> split_bezier_curve(int steps) = 0;
	BezierPoint P0, P1, P2, P3;
};

using Track = std::shared_ptr<byte_track::STrack>;

class TrackerInstance
{
public:
	virtual ~TrackerInstance() = default;
	virtual void init() noexcept = 0;
	virtual void update(const std::vector<Detection>& detections) noexcept = 0;
	byte_track::BYTETracker tracker_instance;
	std::vector<Track> tracks;
	std::mutex tracks_mutex;
};

struct PastTargets
{
	std::vector<Target> targets;
	std::chrono::time_point<std::chrono::high_resolution_clock> time;
};

class ExtrapolatorInstance
{
public:
	virtual ~ExtrapolatorInstance() = default;
	virtual void init() = 0;
	virtual void update(std::vector<Target> new_tracks) = 0;
	virtual void extrapolate(Target& target, std::chrono::time_point<std::chrono::high_resolution_clock> time) = 0;
	std::deque<std::shared_ptr<PastTargets>> past_targets;
	std::mutex past_targets_mutex;
	int extrapolated_x = 0;
	int extrapolated_y = 0;
	int movement_x = 0;
	int movement_y = 0;
};

extern std::shared_ptr<EmulationInstance> emulation;
extern std::shared_ptr<DebugWindowInstance> debugWindow;
extern std::shared_ptr<ConfigInstance> config;
extern std::shared_ptr<AimAssistInstance> aim_assist;
extern std::shared_ptr<AimbotInstance> aimbot;
extern std::shared_ptr<TargetDetectorInstance> target_detector;
extern std::shared_ptr<BezierEngineInstance> bezier_engine;
extern std::shared_ptr<TrackerInstance> tracker;
extern std::shared_ptr<ExtrapolatorInstance> extrapolator;