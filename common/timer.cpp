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

float timer::timerFPS = 0.0f;				// frames
float timer::timerDTInSec = 0.0f;			// in sec
int timer::timerDTInMilliSec = 0;			// in milli sec
float timer::timerElapsedTimeInSec = 0.0f;	// in sec
double timer::timerPreviousTime = 0.0f;
float timer::timerAveragingTime = 0.5f;	 // in sec
int timer::timerFrameCount = 0;
double timer::timerLastTime = 0.0f;
float timer::timerTimeScale = 0.0f;

void timer::init() {
	reset();
}

void timer::update() {
	double curTime = 0.0;
#if PLATFORM == PLATFORM_MAC
	curTime = CFAbsoluteTimeGetCurrent();
#elif defined(GEAR_WINDOWS)
	curTime = (double) timeGetTime() / 1000.0;
#elif PLATFORM == PLATFORM_ANDROID || PLATFORM == PLATFORM_LINUX
	curTime = (double) _getTime() / 1000.0;
#endif
	timerDTInSec = (float) (curTime - timerPreviousTime);
	if (timerDTInSec <= 0.0f) {
		timerDTInSec = 0.03f;
	}
	timerDTInMilliSec = (int) (timerDTInSec * 1000.0f);
	timerElapsedTimeInSec += timerDTInSec;

	if (((float) (curTime - timerLastTime)) >= timerAveragingTime) {
		timerFPS = timerFrameCount / ((float) (curTime - timerLastTime));
		timerFrameCount = 0;
		timerLastTime = curTime;
	} else {
		timerFrameCount++;
	}
	timerPreviousTime = curTime;
}

void timer::update(float targetFPS) {
	double curTime = 0.0;
#if PLATFORM == PLATFORM_MAC
	curTime = CFAbsoluteTimeGetCurrent();
#elif defined(GEAR_WINDOWS)
	curTime = (double) timeGetTime() / 1000.0;
#elif PLATFORM == PLATFORM_ANDROID || PLATFORM == PLATFORM_LINUX
	curTime = (double) _getTime() / 1000.0;
#endif
	timerDTInSec = (float) (curTime - timerPreviousTime);
	if (timerDTInSec <= 0.0f) {
		timerDTInSec = 0.03f;
	}
	while (timerDTInSec < (1.0f / targetFPS)) {
#if PLATFORM == PLATFORM_MAC
		curTime = CFAbsoluteTimeGetCurrent();
#elif defined(GEAR_WINDOWS)
		curTime = (double) timeGetTime() / 1000.0;
#elif PLATFORM == PLATFORM_ANDROID || PLATFORM == PLATFORM_LINUX
		curTime = (double) _getTime() / 1000.0;
#endif
		timerDTInSec = (float) (curTime - timerPreviousTime);
	}
	timerDTInMilliSec = (int) (timerDTInSec * 1000.0f);
	timerElapsedTimeInSec += timerDTInSec;
	timerFPS = 1.0f / timerDTInSec;
	timerPreviousTime = curTime;
}

void timer::reset() {
	timerTimeScale = 1.0f;
	timerFPS = 0.0f;
	timerDTInSec = 0.0f;
	timerDTInMilliSec = 0;
	timerElapsedTimeInSec = 0.0f;
#if PLATFORM == PLATFORM_MAC
	timerPreviousTime = CFAbsoluteTimeGetCurrent();
#elif defined(GEAR_WINDOWS)
	timerPreviousTime = (double) timeGetTime() / 1000.0;
#elif PLATFORM == PLATFORM_ANDROID || PLATFORM == PLATFORM_LINUX
	timerPreviousTime = (double) _getTime() / 1000.0;
#endif
	timerLastTime = timerPreviousTime;
}

double timer::getCurrentTimeInSec() {
#if PLATFORM == PLATFORM_MAC
	return CFAbsoluteTimeGetCurrent();
#elif defined(GEAR_WINDOWS)
	return (double) timeGetTime() / 1000.0;
#elif PLATFORM == PLATFORM_ANDROID || PLATFORM == PLATFORM_LINUX
	return (double) _getTime() / 1000.0;
#endif
	return 0.0;
}

unsigned long timer::getCurrentTimeInMilliSec() {
#if PLATFORM == PLATFORM_MAC
	return CFAbsoluteTimeGetCurrent() * 1000;
#elif defined(GEAR_WINDOWS)
	return timeGetTime();
#elif PLATFORM == PLATFORM_ANDROID || PLATFORM == PLATFORM_LINUX
	return _getTime();
#endif
	return 0;
}
