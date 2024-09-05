//
//  Copyright 2024 homenet25
//  qtimer.cpp
//  networkcommon
//
//  Created by Arun A on 27/10/23.
//

#include "qtimer.hpp"

#include <algorithm>

qtimer_sceduler::qtimer_sceduler() {}

qtimer_sceduler::~qtimer_sceduler() {
	destroy_all();
}
void qtimer_sceduler::destroy_all() {
	for (auto it = timers.cbegin(); it != timers.cend(); it++) {
		qtimer* timer = *it;
		timer->finished = true;
		ev_timer_stop(timer->loop, &timer->timer);
		qtimer_sceduler_data* data = (qtimer_sceduler_data*) timer->data;
		GX_DELETE(data);
		GX_DELETE(timer);
	}
}

void qtimer_sceduler::evtimer_scheduler_cb(qtimer& timer) {
	qtimer_sceduler_data* data = (qtimer_sceduler_data*) timer.data;
	data->TIMEOUT_CALLBACK(timer);
	if (timer.finished) {
		qtimer_sceduler* scheduler = (qtimer_sceduler*) data->scheduler;
		scheduler->destroy_timer(&timer);
	}
}

bool qtimer_sceduler::destroy_timer(qtimer* timer) {
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

void qtimer_sceduler::cancel_timer(qtimer* timer) {
	if (!is_timer_present_in_list(timer)) {
		return;
	}
	timer->finished = true;
	ev_timer_stop(timer->loop, &timer->timer);
}

bool qtimer_sceduler::cancel_and_destroy_timer(qtimer* timer) {
	if (!is_timer_present_in_list(timer)) {
		return false;
	}
	timer->finished = true;
	ev_timer_stop(timer->loop, &timer->timer);
	return destroy_timer(timer);
}

bool qtimer_sceduler::is_timer_present_in_list(qtimer* timer) {
	if (timer == nullptr)
		return false;
	auto result = std::find(timers.begin(), timers.end(), timer);
	return (result != timers.end());
}

qtimer* qtimer_sceduler::schedule_timer(type_qtimer_cb timeout_callback, float delay, void* data) {
	qtimer* timer = DEBUG_NEW qtimer(loop, evtimer_scheduler_cb, DEBUG_NEW qtimer_sceduler_data(data, this, timeout_callback), delay);
	timers.push_back(timer);
	return timer;
}

qtimer* qtimer_sceduler::schedule_count_timer(type_qtimer_cb timeout_callback, float delay, int count, void* data) {
	qtimer* timer = DEBUG_NEW qtimer(loop, evtimer_scheduler_cb, DEBUG_NEW qtimer_sceduler_data(data, this, timeout_callback), delay, count);
	timers.push_back(timer);
	return timer;
}

qtimer* qtimer_sceduler::schedule_repeat_timer(type_qtimer_cb timeout_callback, float delay, void* data) {
	qtimer* timer = DEBUG_NEW qtimer(loop, evtimer_scheduler_cb, DEBUG_NEW qtimer_sceduler_data(data, this, timeout_callback), delay, -1);
	timers.push_back(timer);
	return timer;
}

void qtimer_sceduler::shutdown_mainloop() {
	ev_break(EV_A_ EVBREAK_ONE);
}
