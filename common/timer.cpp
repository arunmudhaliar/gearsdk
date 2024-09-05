//
//  untitled.mm
//  GEARv1.0
//
//  Created by Samarth on 24/06/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#include "timer.hpp"
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

float timer::timer_fps = 0.0f;					// frames
float timer::timer_dt_in_sec = 0.0f;			// in sec
int timer::timer_dt_in_milli_sec = 0;			// in milli sec
float timer::timer_elapsed_time_in_sec = 0.0f;	// in sec
double timer::timer_previous_time = 0.0f;
float timer::timer_averaging_time = 0.5f;  // in sec
double timer::timer_last_time = 0.0f;
int timer::timer_frame_count = 0;
float timer::timer_time_scale = 0.0f;

void timer::init() {
	reset();
}

void timer::update() {
	double cur_time = 0.0;
#if PLATFORM == PLATFORM_MAC
	cur_time = CFAbsoluteTimeGetCurrent();
#elif defined(GEAR_WINDOWS)
	curTime = (double) timeGetTime() / 1000.0;
#elif PLATFORM == PLATFORM_ANDROID || PLATFORM == PLATFORM_LINUX
	curTime = (double) _getTime() / 1000.0;
#endif
	timer_dt_in_sec = (float) (cur_time - timer_previous_time);
	if (timer_dt_in_sec <= 0.0f) {
		timer_dt_in_sec = 0.03f;
	}
	timer_dt_in_milli_sec = (int) (timer_dt_in_sec * 1000.0f);
	timer_elapsed_time_in_sec += timer_dt_in_sec;

	if (((float) (cur_time - timer_last_time)) >= timer_averaging_time) {
		timer_fps = timer_frame_count / ((float) (cur_time - timer_last_time));
		timer_frame_count = 0;
		timer_last_time = cur_time;
	} else {
		timer_frame_count++;
	}
	timer_previous_time = cur_time;
}

void timer::update(float target_fps) {
	double cur_time = 0.0;
#if PLATFORM == PLATFORM_MAC
	cur_time = CFAbsoluteTimeGetCurrent();
#elif defined(GEAR_WINDOWS)
	curTime = (double) timeGetTime() / 1000.0;
#elif PLATFORM == PLATFORM_ANDROID || PLATFORM == PLATFORM_LINUX
	curTime = (double) _getTime() / 1000.0;
#endif
	timer_dt_in_sec = (float) (cur_time - timer_previous_time);
	if (timer_dt_in_sec <= 0.0f) {
		timer_dt_in_sec = 0.03f;
	}
	while (timer_dt_in_sec < (1.0f / target_fps)) {
#if PLATFORM == PLATFORM_MAC
		cur_time = CFAbsoluteTimeGetCurrent();
#elif defined(GEAR_WINDOWS)
		curTime = (double) timeGetTime() / 1000.0;
#elif PLATFORM == PLATFORM_ANDROID || PLATFORM == PLATFORM_LINUX
		curTime = (double) _getTime() / 1000.0;
#endif
		timer_dt_in_sec = (float) (cur_time - timer_previous_time);
	}
	timer_dt_in_milli_sec = (int) (timer_dt_in_sec * 1000.0f);
	timer_elapsed_time_in_sec += timer_dt_in_sec;
	timer_fps = 1.0f / timer_dt_in_sec;
	timer_previous_time = cur_time;
}

void timer::reset() {
	timer_time_scale = 1.0f;
	timer_fps = 0.0f;
	timer_dt_in_sec = 0.0f;
	timer_dt_in_milli_sec = 0;
	timer_elapsed_time_in_sec = 0.0f;
#if PLATFORM == PLATFORM_MAC
	timer_previous_time = CFAbsoluteTimeGetCurrent();
#elif defined(GEAR_WINDOWS)
	timer_previous_time = (double) timeGetTime() / 1000.0;
#elif PLATFORM == PLATFORM_ANDROID || PLATFORM == PLATFORM_LINUX
	timer_previous_time = (double) _getTime() / 1000.0;
#endif
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
