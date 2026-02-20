#ifndef PBRT_UTIL_TIMER_H
#define PBRT_UTIL_TIMER_H

#include "base/base.h"

#include <chrono>
#include <iostream>
#include <string>
#include <unordered_map>

class Timer {
	friend class TimerManager;
	using Clock = std::chrono::high_resolution_clock;
	using TimePoint = Clock::time_point;
public:
	void start() {
		startTime = Clock::now();
		running = true;
	}

	void end() {
		if (!running)return;
		auto endTime = Clock::now();
		float elapsed = std::chrono::duration<float, std::milli>(endTime - startTime).count();
		totalDuration += elapsed;
		count++;
		running = false;

		// Real Time FPS
		++recentCount;
		float timeSinceLastUpdate = std::chrono::duration<float>(endTime - lastFpsUpdateTime).count();
		if (timeSinceLastUpdate >= 1.0f) {
			realtimeFps = recentCount / timeSinceLastUpdate;
			recentCount = 0;
			lastFpsUpdateTime = endTime;
		}
	}

	uint32_t get_count() const { return count; }
	float get_total_duration() const { return totalDuration; }
	float get_average_duration() const { return count > 0 ? totalDuration / count : 0.0f; }
	float get_fps() const { return realtimeFps; }

	float get_delta() {
		auto now = Clock::now();
		float delta = std::chrono::duration<float>(now - lastDeltaTime).count();
		lastDeltaTime = now;
		return delta;
	}
private:
	Timer(){}

	TimePoint startTime;
	float totalDuration = 0.0f;
	uint32_t count = 0;
	bool running = false;

	TimePoint lastFpsUpdateTime = Clock::now();
	uint32_t recentCount = 0;
	float realtimeFps = 0.0f;

	TimePoint lastDeltaTime = Clock::now();
};

class TimerManager {
public:

	Timer& register_timer(const std::string& name) {
		timers.insert({ name, Timer() });
		return timers.find(name)->second;
	}

	Timer& get_timer(const std::string& name) {
		return timers.find(name)->second;
	}

	void print_real_fps(const std::string& name) const {
		auto it = timers.find(name);
		static int last_fps;
		if (it != timers.end()) {
			int current_fps = it->second.get_fps();
			if (last_fps != current_fps) {
				LOG_STREAM("[ Timer:" + name + "] Current FPS: ") << current_fps << std::endl;
				last_fps = current_fps;
			}
			
		}
	}

private:
	std::unordered_map<std::string, Timer> timers;
};

#endif // !PBRT_UTIL_TIMER_H

