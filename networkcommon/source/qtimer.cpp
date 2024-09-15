//
//  Copyright 2024 homenet25
//  qtimer.cpp
//  networkcommon
//
//  Created by Arun A on 27/10/23.
//

#include "qtimer.hpp"

#include <algorithm>

qtimer_scheduler::qtimer_scheduler() {}

qtimer_scheduler::~qtimer_scheduler() {
	destroy_all();
}
void qtimer_scheduler::destroy_all() {
	for (auto it = timers.cbegin(); it != timers.cend(); it++) {
		qtimer* timer = *it;
		timer->finished = true;
		ev_timer_stop(timer->loop, &timer->timer);
		qtimer_sceduler_data* data = (qtimer_sceduler_data*) timer->data;
		GX_DELETE(data);
		GX_DELETE(timer);
	}
}

void qtimer_scheduler::evtimer_scheduler_cb(qtimer& timer) {
	qtimer_sceduler_data* data = (qtimer_sceduler_data*) timer.data;
	data->TIMEOUT_CALLBACK(timer);
	if (timer.finished) {
		qtimer_scheduler* scheduler = (qtimer_scheduler*) data->scheduler;
		scheduler->destroy_timer(&timer);
	}
}

bool qtimer_scheduler::destroy_timer(qtimer* timer) {
	if (!timer->finished) {
		return false;
	}

	size_t old_sz = timers.size();
	timers.erase(std::remove(timers.begin(), timers.end(), timer), timers.end());
	if (old_sz != timers.size()) {
		qtimer_sceduler_data* data = (qtimer_sceduler_data*) timer->data;
		GX_DELETE(data);
		GX_DELETE(timer);
		return true;
	}
	return false;
}

void qtimer_scheduler::cancel_timer(qtimer* timer) {
	if (!is_timer_present_in_list(timer)) {
		return;
	}
	timer->finished = true;
	ev_timer_stop(timer->loop, &timer->timer);
}

bool qtimer_scheduler::cancel_and_destroy_timer(qtimer* timer) {
	if (!is_timer_present_in_list(timer)) {
		return false;
	}
	timer->finished = true;
	ev_timer_stop(timer->loop, &timer->timer);
	return destroy_timer(timer);
}

bool qtimer_scheduler::is_timer_present_in_list(qtimer* timer) {
	if (timer == nullptr)
		return false;
	auto result = std::find(timers.begin(), timers.end(), timer);
	return (result != timers.end());
}

qtimer* qtimer_scheduler::schedule_timer(type_qtimer_cb timeout_callback, float delay, void* data) {
	qtimer* timer = DEBUG_NEW qtimer(loop, evtimer_scheduler_cb, DEBUG_NEW qtimer_sceduler_data(data, this, timeout_callback), delay);
	timers.push_back(timer);
	return timer;
}

qtimer* qtimer_scheduler::schedule_count_timer(type_qtimer_cb timeout_callback, float delay, int count, void* data) {
	qtimer* timer = DEBUG_NEW qtimer(loop, evtimer_scheduler_cb, DEBUG_NEW qtimer_sceduler_data(data, this, timeout_callback), delay, count);
	timers.push_back(timer);
	return timer;
}

qtimer* qtimer_scheduler::schedule_repeat_timer(type_qtimer_cb timeout_callback, float delay, void* data) {
	qtimer* timer = DEBUG_NEW qtimer(loop, evtimer_scheduler_cb, DEBUG_NEW qtimer_sceduler_data(data, this, timeout_callback), delay, -1);
	timers.push_back(timer);
	return timer;
}
