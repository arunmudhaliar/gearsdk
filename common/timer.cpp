//
//  untitled.mm
//  GEARv1.0
//
//  Created by Samarth on 24/06/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#include "timer.hpp"

#include <chrono>
#include <thread>
#if PLATFORM == PLATFORM_MAC
#include <CoreFoundation/CFDate.h>
#elif defined(GEAR_WINDOWS)
#include <Windows.h>
#pragma comment(lib, "Winmm.lib")
#elif PLATFORM == PLATFORM_ANDROID || PLATFORM == PLATFORM_LINUX
#include <sys/time.h>
#include <time.h>
static long _getTime(void) {
	struct timeval now;
	gettimeofday(&now, NULL);
	return (long) (now.tv_sec * 1000 + now.tv_usec / 1000);
}
#else
#error Unknown Platform
#endif

double timer::timer_fps = 0.0;					// frames
double timer::timer_dt_in_sec = 0.0;			// in sec
int timer::timer_dt_in_milli_sec = 0;			// in milli sec
double timer::timer_elapsed_time_in_sec = 0.0;	// in sec
double timer::timer_previous_time = 0.0f;
double timer::timer_averaging_time = 0.5;  // in sec
double timer::timer_last_time = 0.0;
int timer::timer_frame_count = 0;
double timer::timer_time_scale = 0.0;

void timer::init() {
	reset();
}

void timer::update() {
	double cur_time = get_current_time_in_sec();
	timer_dt_in_sec = cur_time - timer_previous_time;
	if (timer_dt_in_sec <= 0.0) {
		timer_dt_in_sec = MIN_FRAME_TIME_SEC;
	}
	timer_dt_in_milli_sec = static_cast<int>(timer_dt_in_sec * 1000.0f);
	timer_elapsed_time_in_sec += timer_dt_in_sec;
	// Calculate FPS only if the averaging time interval has passed
	double time_since_last_avg = cur_time - timer_last_time;
	if (time_since_last_avg >= timer_averaging_time) {
		// Calculate frames per second (FPS)
		timer_fps = timer_frame_count / static_cast<double>(time_since_last_avg);

		timer_frame_count = 0;
		timer_last_time = cur_time;
	} else {
		timer_frame_count++;
	}
	timer_previous_time = cur_time;
}

void timer::update(double target_fps) {
	double cur_time = get_current_time_in_sec();
	timer_dt_in_sec = cur_time - timer_previous_time;
	if (timer_dt_in_sec <= 0.0) {
		timer_dt_in_sec = MIN_FRAME_TIME_SEC;
	}
	while (timer_dt_in_sec < (1.0 / target_fps)) {
		// Sleep for a short duration to prevent busy-waiting
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		cur_time = get_current_time_in_sec();
		timer_dt_in_sec = cur_time - timer_previous_time;
	}
	timer_dt_in_milli_sec = static_cast<int>(timer_dt_in_sec * 1000.0f);
	timer_elapsed_time_in_sec += timer_dt_in_sec;
	timer_fps = 1.0 / timer_dt_in_sec;
	timer_previous_time = cur_time;
}

void timer::reset() {
	timer_time_scale = 1.0;
	timer_fps = 0.0;
	timer_dt_in_sec = 0.0;
	timer_dt_in_milli_sec = 0;
	timer_elapsed_time_in_sec = 0.0;
	timer_previous_time = get_current_time_in_sec();
	timer_last_time = timer_previous_time;
}

double timer::get_current_time_in_sec() {
#if PLATFORM == PLATFORM_MAC
	return CFAbsoluteTimeGetCurrent();
#elif defined(GEAR_WINDOWS)
	return (double) timeGetTime() / 1000.0;
#elif PLATFORM == PLATFORM_ANDROID || PLATFORM == PLATFORM_LINUX
	return (double) _getTime() / 1000.0;
#endif
	return 0.0;
}

unsigned long timer::get_current_time_in_milli_sec() {
#if PLATFORM == PLATFORM_MAC
	return CFAbsoluteTimeGetCurrent() * 1000;
#elif defined(GEAR_WINDOWS)
	return timeGetTime();
#elif PLATFORM == PLATFORM_ANDROID || PLATFORM == PLATFORM_LINUX
	return _getTime();
#endif
	return 0;
}
